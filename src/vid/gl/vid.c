#include "vid.h"
#include "c3model.h"
#include "sx.h"
#include "vid/gl/fbo.h"
#include "file.h"
#include "pngio.h"
#include "bc3io.h"
#include "physics/heli.h"
#include "terrain.h"
#include "matrix4.h"

static unsigned int
compile_src(const char *src, int type)
{
  GLuint shader_obj = glCreateShader(type);

  glShaderSource(shader_obj, 1, &src, NULL);
  glCompileShader(shader_obj);
  // const char *p = "/";
  // glCompileShaderIncludeARB(shader_obj, 1, &p, 0);

  int success;
  glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &success);
  if(!success) {
    char infolog[2048];
    glGetShaderInfoLog(shader_obj, sizeof infolog, NULL, infolog);
    fprintf(stderr, "error compiling shader: %s\n", infolog);
  }

  return shader_obj;
}

static unsigned int
compile_compute_shader(const char *src)
{
  GLuint prog = glCreateProgram();

  unsigned int comp = 0;

  glAttachShader(prog, comp = compile_src(src, GL_COMPUTE_SHADER));
  glLinkProgram(prog);

  int success;
  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if(!success) {
    char infolog[2048];
    glGetProgramInfoLog(prog, sizeof infolog, NULL, infolog);
    fprintf(stderr, "error linking compute shaderprogram: %s\n", infolog);
    prog = -1;
  }

  glDeleteShader(comp);

  return prog;
}


static unsigned int
compile_shader(
    const char *src_vertex,
    const char *src_fragment,
    const char *src_tcs,
    const char *src_tes,
    const char *src_geo)
{
  GLuint prog = glCreateProgram();

  unsigned int vert = 0, frag = 0, tcs = 0, tes = 0, geo = 0;

  glAttachShader(prog, vert = compile_src(src_vertex, GL_VERTEX_SHADER));
  glAttachShader(prog, frag = compile_src(src_fragment, GL_FRAGMENT_SHADER));
  if(src_tcs)
    glAttachShader(prog, tcs = compile_src(src_tcs, GL_TESS_CONTROL_SHADER));
  if(src_tes)
    glAttachShader(prog, tes = compile_src(src_tes, GL_TESS_EVALUATION_SHADER));
  if(src_geo)
    glAttachShader(prog, geo = compile_src(src_geo, GL_GEOMETRY_SHADER));
  glLinkProgram(prog);

  int success;
  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if(!success) {
    char infolog[2048];
    glGetProgramInfoLog(prog, sizeof infolog, NULL, infolog);
    fprintf(stderr, "error linking shaderprogram: %s\n", infolog);
    prog = -1;
  }

  glDeleteShader(vert);
  glDeleteShader(frag);
  if(tcs) glDeleteShader(tcs);
  if(tes) glDeleteShader(tes);
  if(geo) glDeleteShader(geo);

  return prog;
}

static inline void
recompile()
{
#define CC(prog, vert, frag)\
  do {\
  char *vert_shader = file_load("src/vid/gl/shaders/"vert".vert", 0);\
  char *frag_shader = file_load("src/vid/gl/shaders/"frag".frag", 0);\
  assert(vert_shader);\
  assert(frag_shader);\
  sx.vid.program_##prog = compile_shader(\
      vert_shader, frag_shader, 0, 0, 0);\
  free(vert_shader);\
  free(frag_shader); } while(0)
#define CCC(prog, vert, frag, tcs, tes, geo)\
  do {\
  char *vert_shader = file_load("src/vid/gl/shaders/"vert".vert", 0);\
  char *frag_shader = file_load("src/vid/gl/shaders/"frag".frag", 0);\
  char *ttcs_shader = file_load("src/vid/gl/shaders/"tcs".tesc", 0);\
  char *ttes_shader = file_load("src/vid/gl/shaders/"tes".tese", 0);\
  char *geom_shader = file_load("src/vid/gl/shaders/"geo".geom", 0);\
  assert(vert_shader);\
  assert(frag_shader);\
  assert(ttcs_shader);\
  assert(ttes_shader);\
  assert(geom_shader);\
  sx.vid.program_##prog = compile_shader(\
      vert_shader, frag_shader, ttcs_shader, ttes_shader, geom_shader);\
  free(geom_shader);\
  free(ttcs_shader);\
  free(ttes_shader);\
  free(vert_shader);\
  free(frag_shader); } while(0)

  CC(draw_texture, "fstri", "terrain");
  CC(draw_env, "fstri", "env");
  CC(draw_hud, "hud", "hud");
  CC(hud_text, "hud_text", "hud_text");
  CC(taa, "fstri", "taa");
  CC(blit_texture, "fstri", "blit");
  CC(grade, "fstri", "hud_horizon");
  CC(hero, "hero", "hero");
  CC(debug_line, "line", "line");
  CC(debug_flow, "flow", "line");
  CCC(terrain, "ground", "ground", "ground", "ground", "ground");
  // CC(compute_flow, "fstri", "flowcomp");
#undef CC
#undef CCC
  char *comp = file_load("src/vid/gl/shaders/flow.comp", 0);
  sx.vid.program_compute_flow = compile_compute_shader(comp);
  free(comp);
}

static void
gl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const char *msg, GLvoid *param)
{
  const char *ssource;
  switch(source) {
    case 0x8246: ssource = "API            "; break;
    case 0x8247: ssource = "WINDOW_SYSTEM  "; break;
    case 0x8248: ssource = "SHADER_COMPILER"; break;
    case 0x8249: ssource = "THIRD_PARTY    "; break;
    case 0x824A: ssource = "APPLICATION    "; break;
    case 0x824B: ssource = "OTHER          "; break;
    default:     ssource = "INVALID        "; break;
  }

  const char *stype;

  switch(type) {
    case 0x824C: stype = "ERROR              "; break;
    case 0x824D: stype = "DEPRECATED_BEHAVIOR"; break;
    case 0x824E: stype = "UNDEFINED_BEHAVIOR "; break;
    case 0x824F: stype = "PORTABILITY        "; break;
    case 0x8250: stype = "PERFORMANCE        "; break;
    case 0x8251: stype = "OTHER              "; break;
    default:     stype = "INVALID            "; break;
  }

  const char *sseverity;

  switch(severity) {
    case 0x9146: sseverity = "HIGH   "; break;
    case 0x9147: sseverity = "MEDIUM "; break;
    case 0x9148: sseverity = "LOW    "; break;
    default:     sseverity = "INVALID"; break;
  }

  fprintf(stderr, "%s %s %s: %s\n", sseverity, stype, ssource, msg);
}

int sx_vid_init(
  uint32_t width,
  uint32_t height)
{
  sx_vid_t *vid = &sx.vid;
  memset(vid, 0, sizeof(sx.vid));
  // get drawable surface from SDL2
  SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // ??
  // SDL_GL_SetSwapInterval(0); // vsync on // does nothing

  vid->window = SDL_CreateWindow("sioux", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
  if(!vid->window) return 1;
  vid->context = SDL_GL_CreateContext(vid->window);
  if(!vid->context) return 1;

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if(err != GLEW_OK)
  {
    fprintf(stderr,  "[vid] %s\n", glewGetErrorString(err));
    return 1;
  }
  while(glGetError() != GL_NO_ERROR);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glEnable(GL_DEBUG_OUTPUT);

  glDebugMessageCallback((GLDEBUGPROC) gl_error_callback, NULL);
  //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
  GLuint mask_ids[] = {
    131076, // Generic Vertex Attrib foo
    131185, // memory type for buffer operations
  };
  glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE,
      sizeof(mask_ids) / sizeof(mask_ids[0]), mask_ids, GL_FALSE);

  fbo_init_t fi = {{0}};
  fi.color[0] = GL_RGBA16F; // colour
  fi.color[1] = GL_RG16F;   // motion vectors
  fi.width = width;
  fi.height = height;
  // fi.depth = GL_DEPTH_COMPONENT16;
  fi.depth = GL_DEPTH_COMPONENT32F;
  // full screen buffers for taa and post:
  sx.vid.fbo0 = fbo_init(&fi);
  sx.vid.fbo1 = fbo_init(&fi);
  // terrain buffer is mip mapped, so we can use them for diffuse lighting
  fi.color_mip = 1;
  sx.vid.fsb  = fbo_init(&fi);

  fi.color_mip = 0;
  fi.color[1] = GL_RGB16F;   // motion vectors + depth
  // terrain can be reduced res, will be upscaled
  fi.width  = width; // /rt.rt_lod;
  fi.height = height; // /rt.rt_lod;
  // environment will be ray traced into this:
  sx.vid.raster = fbo_init(&fi);
  sx.vid.fbo = sx.vid.fbo0;

  // init cube map for fisheye geo
  fi.width = 1.3*height;  // expand a bit for better resolution
  fi.height = 1.3*height; // because we'll resample it
  fi.color_mip = 0;
  fi.depth = GL_DEPTH_COMPONENT32F;
  fi.color_mip = 0;
  for(int k=0;k<2;k++)
    sx.vid.cube[k] = fbo_init(&fi);

  // we won't use the depth textures for shadow mapping:
  glBindTexture(GL_TEXTURE_2D, sx.vid.raster->tex_depth);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
  glBindTexture(GL_TEXTURE_2D, sx.vid.cube[0]->tex_depth);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
  glBindTexture(GL_TEXTURE_2D, sx.vid.cube[1]->tex_depth);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

  recompile();
  glGenVertexArrays(1, &sx.vid.vao_empty);

  // debug lines
  glGenVertexArrays(1, &sx.vid.vao_debug_line);
  glBindVertexArray(sx.vid.vao_debug_line);
  glGenBuffers(1, &sx.vid.vbo_debug_line);
  glBindBuffer(GL_ARRAY_BUFFER, sx.vid.vbo_debug_line);
  glNamedBufferStorage(sx.vid.vbo_debug_line, sizeof(sx.vid.debug_line_vx), sx.vid.debug_line_vx, GL_DYNAMIC_STORAGE_BIT);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexArrayAttrib(sx.vid.vao_debug_line, 0);
  glBindAttribLocation(sx.vid.program_debug_line, 0, "position");

  // tessellated terrain for rasterisation:
  sx_vid_gl_terrain_init(
      &sx.vid.terrain_vx,
      &sx.vid.terrain_idx,
      &sx.vid.terrain_vx_cnt,
      &sx.vid.terrain_idx_cnt);

  glGenVertexArrays(1, &sx.vid.vao_terrain);
  glBindVertexArray(sx.vid.vao_terrain);
  glGenBuffers(2, sx.vid.vbo_terrain);
  glBindBuffer(GL_ARRAY_BUFFER, sx.vid.vbo_terrain[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*sx.vid.terrain_vx_cnt, sx.vid.terrain_vx, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexArrayAttrib(sx.vid.vao_terrain, 0);
  glBindAttribLocation(sx.vid.program_terrain, 0, "position");
  // index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sx.vid.vbo_terrain[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*sx.vid.terrain_idx_cnt*4, sx.vid.terrain_idx, GL_STATIC_DRAW);

  // heads up display:
  glGenVertexArrays(1, &sx.vid.vao_hud);
  glBindVertexArray(sx.vid.vao_hud);
  glGenBuffers(1, &sx.vid.vbo_hud);

  glBindBuffer(GL_ARRAY_BUFFER, sx.vid.vbo_hud);

  glNamedBufferStorage(sx.vid.vbo_hud, sizeof(sx.vid.hud.lines), sx.vid.hud.lines, GL_DYNAMIC_STORAGE_BIT);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glEnableVertexArrayAttrib(sx.vid.vao_hud, 0);
  glBindAttribLocation(sx.vid.program_draw_hud, 0, "position");

  // text for heads up display:
  glGenVertexArrays(1, &sx.vid.vao_hud_text);
  glBindVertexArray(sx.vid.vao_hud_text);
  glGenBuffers(3, sx.vid.vbo_hud_text);

  glBindBuffer(GL_ARRAY_BUFFER, sx.vid.vbo_hud_text[0]);
  glNamedBufferStorage(sx.vid.vbo_hud_text[0], sizeof(sx.vid.hud_text_vx), sx.vid.hud_text_vx, GL_DYNAMIC_STORAGE_BIT);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexArrayAttrib(sx.vid.vao_hud_text, 0);
  glBindAttribLocation(sx.vid.program_hud_text, 0, "position");

  glBindBuffer(GL_ARRAY_BUFFER, sx.vid.vbo_hud_text[1]);
  glNamedBufferStorage(sx.vid.vbo_hud_text[1], sizeof(sx.vid.hud_text_uv), sx.vid.hud_text_uv, GL_DYNAMIC_STORAGE_BIT);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexArrayAttrib(sx.vid.vao_hud_text, 1);
  glBindAttribLocation(sx.vid.program_hud_text, 1, "uvs");

  // index buffer
  for(int i=0;i<sizeof(sx.vid.hud_text_id)/sizeof(sx.vid.hud_text_id[0])/6;i++)
  {
    sx.vid.hud_text_id[6*i+0] = 4*i+2;
    sx.vid.hud_text_id[6*i+1] = 4*i+1;
    sx.vid.hud_text_id[6*i+2] = 4*i+0;
    sx.vid.hud_text_id[6*i+3] = 4*i+3;
    sx.vid.hud_text_id[6*i+4] = 4*i+2;
    sx.vid.hud_text_id[6*i+5] = 4*i+0;
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sx.vid.vbo_hud_text[2]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sx.vid.hud_text_id), sx.vid.hud_text_id, GL_STATIC_DRAW);

  sx_vid_init_image("assets/hud-font.png", &sx.vid.hud_text_font);
  sx.vid.hud_text_chars = 0;

  sx.vid.joystick = 0;
  if(SDL_NumJoysticks() > 0)
    sx.vid.joystick = SDL_JoystickOpen(0);
  if(sx.vid.joystick)
  {
    fprintf(stderr, "[vid] found joystick %s with %d axes %d buttons %d balls\n",
        SDL_JoystickName(0),
        SDL_JoystickNumAxes(sx.vid.joystick),
        SDL_JoystickNumButtons(sx.vid.joystick),
        SDL_JoystickNumBalls(sx.vid.joystick));
  }

  // init instance buffers (mat4 mv, mvp)
  glGenBuffers(1, &sx.vid.geo_instance_ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, sx.vid.geo_instance_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 320000, 0, GL_STREAM_DRAW);

  // init flow textures
  sx.vid.tex_flow_cnt = 2;
  glGenTextures(sx.vid.tex_flow_cnt, sx.vid.tex_flow);
  const int wd = 128;
  float *buf = malloc(sizeof(float)*wd*wd*wd*3);
  for(int k=0;k<wd*wd*wd;k++)
  {
    buf[3*k+0] = -10;
    buf[3*k+1] = 0;
    buf[3*k+2] = 0;
  }
  for(int k=0;k<sx.vid.tex_flow_cnt;k++)
  {
#if 1
    glBindTexture(GL_TEXTURE_2D, sx.vid.tex_flow[k]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    float col[3] = {0,0,0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, col);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 1024, 0, GL_RGB, GL_FLOAT, buf);
#else
    glBindTexture(GL_TEXTURE_3D, sx.vid.tex_flow[k]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    float col[3] = {0,0,0};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, col);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, wd, wd, wd, 0, GL_RGB, GL_FLOAT, buf);
#endif
  }
  free(buf);
  sx.vid.tex_flow_curr = 0;

  return 0;
}

void sx_vid_start_mission()
{
  // mip maps uploaded and such?
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void sx_vid_end_mission()
{
  // TODO: reset counts to 0, maybe free some memory
}

void sx_vid_cleanup()
{
  if(sx.vid.joystick)
    SDL_JoystickClose(sx.vid.joystick);
  SDL_DestroyWindow(sx.vid.window);
  SDL_Quit();
}

// init renderable geo, return handle or -1
uint32_t sx_vid_init_geo(const char *filename, float *aabb)
{
  // TODO find if geo already loaded and return i!
  uint32_t i = sx.vid.num_geo++;
  assert(i < sizeof(sx.vid.geo)/sizeof(sx.vid.geo[0]));
  uint64_t len;
  c3m_header_t *h = file_load(filename, &len);
  if(!h) return -1;
  // assert(h->off_eof == len); // this is mostly true but sometimes 8 bytes go missing?

  sx_geo_t *g = sx.vid.geo + i;
  memset(g, 0, sizeof(sx_geo_t));

  strncpy(g->filename, filename, sizeof(g->filename)-1);

  // get basic geo layout stuff:
  float *vertices, *normals, *uvs;
  uint32_t *indices;
  c3m_to_float_arrays(h, &vertices, &normals, &uvs, &indices,
      &g->num_vtx, &g->num_nor, &g->num_uvs, &g->num_idx);

  // get aabb from header of c3m. it's not always accurate, maybe there's a reason other than rounding (hit boxes?)
  if(aabb) c3m_get_aabb(h, aabb);

  // now walk textures and init images
  g->num_mat = c3m_num_materials(h);
  if(g->num_mat)
  {
    uint32_t tu = 1; // texture unit
    uint32_t *buf = malloc(sizeof(uint32_t) * g->num_mat * 8);
    memset(buf, 0, sizeof(uint32_t)*g->num_mat*8);
    for(int m=0;m<g->num_mat;m++)
    {
      c3m_material_t *mt = c3m_get_materials(h)+m;
      g->mat[m].texname[0] = 0;
      strncpy(g->mat[m].texname, mt->texname, sizeof(g->mat[m].texname));
      g->mat[m].texu = -1;
      g->mat[m].texid[0] = -1;
      g->mat[m].tex_cnt = 0;
      if(g->mat[m].texname[0])
      {
        for(int i=0;i<m;i++) if(!strcmp(g->mat[m].texname, g->mat[i].texname))
        {
          g->mat[m].texu = g->mat[i].texu;
          memcpy(g->mat[m].texid, g->mat[i].texid, sizeof(g->mat[m].texid));
          g->mat[m].tex_cnt = g->mat[i].tex_cnt;
          break;
        }
        if(g->mat[m].texu == -1)
        {
          g->mat[m].tex_cnt = sx_vid_init_image(g->mat[m].texname, g->mat[m].texid);
          g->mat[m].texu = tu++;
        }
      }
      buf[m*8 + 0] = g->mat[m].texu;
      buf[m*8 + 1] = g->mat[m].dunno[0] = mt->dunno[0];
      buf[m*8 + 2] = g->mat[m].dunno[1] = mt->dunno[1];
      buf[m*8 + 3] = g->mat[m].dunno[2] = mt->dunno[2];
      buf[m*8 + 4] = g->mat[m].dunno[3] = mt->dunno[3];
    }
    // upload material info as texture
    glGenTextures(1, &g->mat_tex);
    glBindTexture(GL_TEXTURE_2D, g->mat_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 8, g->num_mat);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 8, g->num_mat, GL_RGBA,
        GL_UNSIGNED_BYTE, buf);
    free(buf);
  }

  // XXX get from considering contents of input model?
  // XXX need to compile shaders or hold list of them
  g->program = sx.vid.program_hero;// program;

  if(g->program == -1) return -1; // compile error

  glGenVertexArrays(1, &g->vaid);
  glBindVertexArray(g->vaid);

  glGenBuffers(4, g->vbo);

  glBindBuffer(GL_ARRAY_BUFFER, g->vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, g->num_vtx * sizeof(float)*3, vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexArrayAttrib(g->vaid, 0);
  glBindAttribLocation(g->program, 0, "position");

  glBindBuffer(GL_ARRAY_BUFFER, g->vbo[1]);
  glBufferData(GL_ARRAY_BUFFER, g->num_nor * sizeof(float)*3, normals, GL_STATIC_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexArrayAttrib(g->vaid, 1);
  glBindAttribLocation(g->program, 1, "normal");

  glBindBuffer(GL_ARRAY_BUFFER, g->vbo[2]);
  glBufferData(GL_ARRAY_BUFFER, g->num_uvs * sizeof(float)*3, uvs, GL_STATIC_DRAW);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexArrayAttrib(g->vaid, 2);
  glBindAttribLocation(g->program, 2, "uvs");

  // index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g->vbo[3]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, g->num_idx * sizeof(uint32_t), indices, GL_STATIC_DRAW);

  free(vertices);
  free(normals);
  free(uvs);
  free(indices);
  free(h);

  return i;
}

void sx_vid_hud_text_clear()
{
  sx.vid.hud_text_chars = 0;
}

uint32_t
sx_vid_hud_text(
    const char *text, uint32_t bufpos,
    float x, float y,
    int center_x, int center_y)
{
  const float char_wd = 20.0f/sx.width, char_ht = 30.0f/sx.height; // TODO: params?

  const uint32_t num_chars = strlen(text);
  float *vertices = sx.vid.hud_text_vx + 8*bufpos;
  float *uvs = sx.vid.hud_text_uv + 8*bufpos;

  float wd = char_wd * num_chars;
  float ht = char_ht;
  float ox = center_x ? -.5f*wd : 0.0f;
  float oy = center_y ? -.5f*ht : 0.0f;

  assert((bufpos+num_chars)*8 <= sizeof(sx.vid.hud_text_vx)/sizeof(float));

  // if i wasn't so lazy i'd clean this up
  for(int i=0;i<num_chars;i++)
  {
    vertices[8*i+0] = ox + x + char_wd * i;
    vertices[8*i+1] = oy + y;
    vertices[8*i+2] = ox + x + char_wd * (1+i);
    vertices[8*i+3] = oy + y;
    vertices[8*i+4] = ox + x + char_wd * (1+i);
    vertices[8*i+5] = oy + y + char_ht;
    vertices[8*i+6] = ox + x + char_wd * i;
    vertices[8*i+7] = oy + y + char_ht;

    int cpos = 36; // '-'
    if(text[i] >= 'A' && text[i] <= 'Z') cpos = 10 + text[i] - 'A';
    else if(text[i] >= 'a' && text[i] <= 'z') cpos = 10 + text[i] - 'a';
    else if(text[i] >= 48 && text[i] <= 57) cpos = text[i] - '0';
    uvs[8*i+0] = cpos / 38.0f;
    uvs[8*i+1] = 1.0f;
    uvs[8*i+2] = (cpos+1) / 38.0f;
    uvs[8*i+3] = 1.0f;
    uvs[8*i+4] = (cpos+1) / 38.0f;
    uvs[8*i+5] = 0.0f;
    uvs[8*i+6] = cpos / 38.0f;
    uvs[8*i+7] = 0.0f;
  }
  sx.vid.hud_text_chars = MAX(sx.vid.hud_text_chars, bufpos + num_chars);
  return sx.vid.hud_text_chars;
}

// load terrain textures
int sx_vid_init_terrain(
    const char *col,  const char *hgt,  const char *mat,
    const char *ccol, const char *chgt, const char *cmat)
{
  const char *filename[6] = {col, hgt, mat, ccol, chgt, cmat};

  glGenTextures(8, sx.vid.tex_terrain);
  for(int k=0;k<6;k++)
  {
    glBindTexture(GL_TEXTURE_2D, sx.vid.tex_terrain[k]);

    int width, height, bpp;
    uint8_t *buf;
    char fn[256];
    png_from_pcx(filename[k], fn, sizeof(fn));
    if(png_read(fn, &width, &height, (void**)&buf, &bpp))
    {
      fprintf(stderr, "[vid] failed to open `%s'\n", filename[k]);
      return 1;
    }
    // TODO: we want these in 16 bits if we can!
    assert(bpp == 8);
    if(k == 1 || k == 4) // the displacement channels
      for(int j=0;j<height;j++) for(int i=0;i<width;i++)
        buf[j*width+i] = buf[4*(j*width+i)];

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if(k == 1 || k == 4 || k == 0 || k == 3) // colour and displacement buffers
    // if(1)//if(k == 0 || k == 3) // colour buffers
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      if(k == 1)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    }
    else
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    if(k != 1 && k != 4)
    {
      glTexStorage2D(GL_TEXTURE_2D, 1,
          (k == 0 || k == 3) ?
          GL_SRGB8_ALPHA8 : GL_RGBA8,
          width, height);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
          GL_UNSIGNED_BYTE, buf);
    }
    else if(k == 1 || k == 4) // displacements
    {
      // upload original texture
      int num_mipmaps = 1;
      if(k == 1) num_mipmaps = 10;
      glTexStorage2D(GL_TEXTURE_2D, num_mipmaps, GL_R8, width, height);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, buf);

      // generate mip maps
      if(num_mipmaps > 1)
        glGenerateMipmap(GL_TEXTURE_2D);

      // also do some mip mapping for ray tracing
      if(k == 1)
        glBindTexture(GL_TEXTURE_2D, sx.vid.tex_terrain[6]);
      if(k == 4)
        glBindTexture(GL_TEXTURE_2D, sx.vid.tex_terrain[7]);


      int max_l = k == 1 ? 10 : 4;
      glTexStorage2D(GL_TEXTURE_2D, max_l+1, GL_R8, width, height);
      // level 0 is same res but max of 2x2 blocks:
      uint8_t *buf2 = malloc(sizeof(uint8_t)*width*height);
      for(int j=0;j<height;j++) for(int i=0;i<width;i++)
      {
        buf2[j*width+i] = MAX(
            MAX(buf[((j+0)%height)*width+((i+0)%width)],
                buf[((j+0)%height)*width+((i+1)%width)]),
            MAX(buf[((j+1)%height)*width+((i+0)%width)],
                buf[((j+1)%height)*width+((i+1)%width)]));
      }
      for(int l=0;l<=max_l;l++)
      {
        glTexSubImage2D(GL_TEXTURE_2D, l, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, buf2);
        height /= 2;
        width  /= 2;
        for(int j=0;j<height;j++) for(int i=0;i<width;i++)
        {
          buf2[j*width+i] = MAX(
              MAX(buf2[(2*j+0)*(2*width)+2*i+0],
                  buf2[(2*j+0)*(2*width)+2*i+1]),
              MAX(buf2[(2*j+1)*(2*width)+2*i+0],
                  buf2[(2*j+1)*(2*width)+2*i+1]));
        }
      }
      free(buf2);
    }

    free(buf);
    fprintf(stderr, "[vid] loaded terrain texture `%s'\n", filename[k]);
  }
  return 0;
}

// load texture file, return texture count or 0
uint32_t sx_vid_init_image(const char *filename, uint32_t *texid)
{
  if(filename[0] == 0) return 0;
  // mangle from PCX to lowercase png
  char fn[128];
  png_from_pcx(filename, fn, sizeof(fn));

  int cnt = 0; // how many textures in an animation?
  // apparently a '0' suffix means that the texture will be animated.
  // count trailing zeros:
  uint32_t z = 0;
  int len = strlen(fn);
  // sometimes it's also the case that the first frame is 01, unfortunately.
  // but this way of detecting it fails because there are plenty of 01 02 variants
  // that are not at all animations :/
  int off = 0;
  // if(fn[len-5] == '0' || fn[len-5] == '1') z = 1;
  // if(fn[len-5] == '1') off = 1;
  if(fn[len-5] == '0') z = 1;
  while(len-5-z > 0 && fn[len-5-z] == '0') z++;
  fn[len-4-z] = 0;

  char pattern[150] = {0};
  if(z > 0) snprintf(pattern, sizeof(pattern), "%s%%0%uu.bc3", fn, z);
  else      snprintf(pattern, sizeof(pattern), "%s.bc3", fn);
  int prev_wd = 0, prev_ht = 0;
  for(int i=0;i<32;i++)
  { // don't go over memory bound, max textures is 32
    snprintf(fn, sizeof(fn), pattern, i+off);

    int width, height, bpp = 8;
    uint8_t *buf;
    // int err = png_read(fn, &width, &height, (void**)&buf, &bpp);
    int err = bc3_read(fn, &width, &height, &buf);
    if(err && (cnt == 0))
    {
      fprintf(stderr, "[vid] failed to open `%s' (%d)\n", fn, err);
      return 0;
    }
    else if(err && (cnt > 0)) return cnt;
    assert(bpp == 8);

    if(i > off && (prev_wd != width || prev_ht != height))
    { // apparently not an animated texture after all
      free(buf);
      return cnt;
    }
    prev_wd = width;
    prev_ht = height;

    glGenTextures(1, texid+i);
    glBindTexture(GL_TEXTURE_2D, texid[i]);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0,
        // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
        GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,
        width, height, 0, 
        16*((width+3)/4)*((height+3)/4), buf);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    free(buf);
    cnt++;
    // fprintf(stderr, "[vid] loading texture `%s'\n", fn);
    if(!z) break;
  }
  return cnt;
}


// compute model view projection matrix as
// MVP = P * R * F * V * M * C3M
// where
// C3M: comanche 3 model space to our local space flip axes
// M  : model matrix to world space
// V  : to camera space
// F  : flip axes for opengl such that -z inside screen
// R  : rotate to left and right cube sides if rendering fisheye
// P  : projection matrix
void
sx_vid_compute_mvp(
    float        *MVP,
    float        *MV,
    const int     u_cube_side,
    const quat_t *q_model,   // orientation of model (model to world quaternion)
    const float  *p_model,   // world space position of model coordinate system
    sx_camera_t  *cam,
    int c3model)
{
  // C3M
  // if c3 model coordinates, need to flip to our local model coordinates as
  // used for physics:
  float C3M[16] = {
    -1,  0, 0, 0,
     0,  0, 1, 0,
     0, -1, 0, 0,
     0,  0, 0, 1,
  };
  if(!c3model) mat4_set_identity(C3M);

  // M: model to world space. need quaternion and world space position of
  // model.
  float mat3[9], M[16];
  quat_to_mat3(q_model, mat3);
  mat4_from_mat3(M, mat3, p_model);

  // V: world to view space
  float c_pos[3] = {-cam->x[0], -cam->x[1], -cam->x[2]};
  quat_t vq = cam->q;
  quat_conj(&vq);
  quat_transform(&vq, c_pos);

  float V[16];
  quat_to_mat3(&vq, mat3);
  mat4_from_mat3(V, mat3, c_pos);

  float F[16] = {
    -1, 0,  0, 0,
     0, 1,  0, 0,
     0, 0, -1, 0,
     0, 0,  0, 1};
  // now coord system is:
  // x-right, y-up, z-out of screen (negative z into it, right handed)

  // need to rotate left or right just enough to cover half the total field of view:
  // looks correct when assuming +z forward, +x right:
  const float s = sin(cam->hfov*0.25) * (u_cube_side == 0 ? 1.0 : -1.0);
  const float c = cos(cam->hfov*0.25);
  float R[] = {
    c, 0, -s, 0,
    0, 1,  0, 0,
    s, 0,  c, 0,
    0, 0,  0, 1};
  if(u_cube_side == -1) mat4_set_identity(R);

  // now scale to accomodate field of view:
  const float n = .10, f = 10000.0;
  const float r = u_cube_side >= 0 ?
    n*fabsf(s/c) :
    n*tan(cam->hfov*0.5),
    l = -r;
  // need some leeway to not cut off corners:
  const float t = u_cube_side >= 0 ?
    1.38 * n*tan(cam->vfov*0.5) :
    sx.height * r / sx.width, // n * tan(cam->vfov*0.5),
    b = -t;
  float P[] = {
    2*n/(r-l), 0, (r+l)/(r-l), 0,
    0, 2*n/(t-b), (t+b)/(t-b), 0,
    0, 0,        -(f+n)/(f-n), -2*f*n/(f-n),
    0, 0,                  -1, 0};

  // MVP = P * R * F * V * M * C3M
  float tmp[16];
  mat4_mul(M, C3M, tmp); // M * C3
  mat4_mul(V, tmp, M);   // V * MC3
  mat4_mul(F, M, tmp);   // F * VMC3
  mat4_mul(R, tmp, MV);  // MV  = R * FVMC3
  mat4_mul(P, MV, MVP);  // MVP = P * RFVMC3
}

void
sx_vid_push_geo_instance(
    const uint32_t gi,
    const float  *omp,
    const quat_t *omq,
    const float  *mp,
    const quat_t *mq)
{
  if(gi < 0 || gi >= sx.vid.num_geo) return;
  sx_geo_t *g = sx.vid.geo + gi;

  if(g->instances > 1024) return;
  int iid = g->instances++;
  // specific to this geo/entity
  float *MV  = g->instance_mat + 32*iid;
  float *MVP = g->instance_mat + 32*iid + 16;
  sx_vid_compute_mvp(MVP, MV, sx.vid.cube_side, mq, mp, &sx.cam, 1);
}

void
sx_vid_render_instanced_geo(
    const uint32_t gi)
{
  if(gi < 0 || gi >= sx.vid.num_geo) return;
  sx_geo_t *g = sx.vid.geo + gi;

  // all geo use this ubershader:
  uint32_t program = sx.vid.program_hero;

  // specific to this geo/entity
  glUniform1ui(glGetUniformLocation(program, "u_instance_offset"), g->instance_offset);

  // bind textures from geo material with given index
  int tu = 0;
  for(int m=0;m<g->num_mat;m++) if(g->mat[m].texu > tu)
  {
    tu = g->mat[m].texu; // only if not already bound by earlier material
    uint32_t ti = (sx.time/250) % g->mat[m].tex_cnt;
    if(g->mat[m].texid[ti] != -1)
      glBindTextureUnit(g->mat[m].texu, g->mat[m].texid[ti]);
  }

  glBindTextureUnit(9, g->mat_tex);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sx.vid.geo_instance_ssbo);
  glBindVertexArray(g->vaid);
  glDrawElementsInstanced(GL_TRIANGLES, g->num_idx, GL_UNSIGNED_INT, 0, g->instances);
}

void
sx_vid_render_geo(
    const uint32_t gi,
    const float  *omp,
    const quat_t *omq,
    const float  *mp,
    const quat_t *mq)
{
  if(gi < 0 || gi >= sx.vid.num_geo) return;
  sx_geo_t *g = sx.vid.geo + gi;

  // all geo use this ubershader:
  uint32_t program = sx.vid.program_hero;

  // specific to this geo/entity
  float MVP[16], MV[16];
  sx_vid_compute_mvp(MVP, MV, sx.vid.cube_side, mq, mp, &sx.cam, 1);
  glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_TRUE, MVP);
  glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"),  1, GL_TRUE, MV);

  // bind textures from geo material with given index
  int tu = 0;
  for(int m=0;m<g->num_mat;m++) if(g->mat[m].texu > tu)
  {
    tu = g->mat[m].texu; // only if not already bound by earlier material
    uint32_t ti = (sx.time/250) % g->mat[m].tex_cnt;
    if(g->mat[m].texid[ti] != -1)
      glBindTextureUnit(g->mat[m].texu, g->mat[m].texid[ti]);
  }

  glBindTextureUnit(9, g->mat_tex);
  glBindVertexArray(g->vaid);
  glDrawElements(GL_TRIANGLES, g->num_idx, GL_UNSIGNED_INT, 0);
}

// compute matrix from 3d flow texture space aligned with
// model to world space position
static inline void
get_flow_matrix(
    const quat_t *q,
    const float  *p,
    float        *M)
{
  // 3D texture around helicopter position to world space
  // changing this has implications on flow.{comp,vert}
  float wd = 80; // in meters: diameter around object
  float x[][3] = { // u v w o
    {wd, 0, 0},
    {0, wd, 0},
    {0, 0, wd},
    {-wd/2, -wd/2, -wd/2},
  };
  for(int k=0;k<4;k++)
  { // transform to world space
    quat_transform(q, x[k]);
    for(int i=0;i<3;i++) x[k][i] += p[i];
  }
  M[ 0] = x[0][0]; M[ 1] = x[1][0]; M[ 2] = x[2][0]; M[ 3] = x[3][0]; 
  M[ 4] = x[0][1]; M[ 5] = x[1][1]; M[ 6] = x[2][1]; M[ 7] = x[3][1];
  M[ 8] = x[0][2]; M[ 9] = x[1][2]; M[10] = x[2][2]; M[11] = x[3][2];
  M[12] = 0;       M[13] = 0;       M[14] = 0;       M[15] = 1;
}

void sx_vid_render_frame_rect()
{
#if 0
  // XXX this seems stupid beyond repair. maybe just run a fragment shader instead?
  // execute air flow simulation compute shader
  if(sx.vid.program_compute_flow != -1)
  {
    const sx_entity_t *ent = sx.world.entity + sx.world.player_entity;
    float Mp[16], Mc[16], MpI[16], M[16]; // model to world
    get_flow_matrix(&ent->body.q, ent->body.c, Mc);
    get_flow_matrix(&ent->prev_q, ent->prev_x, Mp);
    mat4_inv(Mp, MpI);
    mat4_mul(MpI, Mc, M);
    // fprintf(stderr, "%g %g %g -> %g %g %g\n",
    //     ent->body.c[0], ent->body.c[1], ent->body.c[2],
    //     ent->prev_x[0], ent->prev_x[1], ent->prev_x[2]);

    uint32_t program = sx.vid.program_compute_flow;
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mat"), 1, GL_TRUE, M);
    // glUniformMatrix4fv(glGetUniformLocation(program, "u_curr"), 1, GL_TRUE, Mc);
    glUniform1f(glGetUniformLocation(program, "u_alpha"), 0.5);
    int c = sx.vid.tex_flow_curr;
    int n = (c+1)%sx.vid.tex_flow_cnt;
    glBindTextureUnit(0, sx.vid.tex_flow[c]);
    // glBindTextureUnit(1, sx.vid.tex_flow[n]);
    // glUniform1i(glGetUniformLocation(program, "new_flow"), 1);
    glBindImageTexture(1, sx.vid.tex_flow[n], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    sx.vid.tex_flow_curr = n;
#if 1
    glDispatchCompute(2048/8, 1024/8, 1);//128/4, 128/4, 128/4);
#else
    glUniform2i(glGetUniformLocation(program, "u_res"), sx.width, sx.height);
    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
#endif
    // mem barrier only needed before flow is drawn?
    // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT); // barrier for next reader
    // glMemoryBarrier(GL_ALL_BARRIER_BITS);
    // glFinish();
    // glFlush();
  }
#endif

  // glClearColor(0, 0, 0, 1);
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  sx.vid.geo_instance_cnt = 0;

  fbo_t *old_fbo = sx.vid.fbo == sx.vid.fbo0 ? sx.vid.fbo1 : sx.vid.fbo0;

  // half angle of horizontal field of view. pi/2 means 180 degrees total
  // and is the max we can get with only two cubemap sides

  // rasterise geo before ray tracing
  if(sx.vid.program_hero != -1)
  {
    // TODO: get from world
    // float sunv[] = {-0.624695,0.468521,-0.624695};
    // normalise(sunv);
    // quat_transform_inv(&sx.cam.q, sunv); // sun in camera space

    sx.vid.cube_side = -1;
    fbo_bind(sx.vid.raster);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

#if 1
    // render terrain first and hope for early z out?
    if(sx.vid.program_terrain != -1)
    {
      uint32_t program = sx.vid.program_terrain;
      glUseProgram(program);
      glUniform1f(glGetUniformLocation(program, "u_time"), sx.time / 1000.0f);
      glUniform1f(glGetUniformLocation(program, "u_lod"), 1);
      glUniform2f(glGetUniformLocation(program, "u_res"), sx.width, sx.height);
      glUniform1i(glGetUniformLocation(program, "u_frame"), sx.time);
      glUniform3f(glGetUniformLocation(program, "u_pos_ws"),
          sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);
      glUniform2f(glGetUniformLocation(program, "u_terrain_bounds"),
          sx.world.terrain_low, sx.world.terrain_high);

      // terrain comes in world space but centered at camera position
      float MVP[16], MV[16];
      quat_t mq;
      quat_init_angle(&mq, 0, 1, 0, 0);
      float mp[] = {sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]};
      sx_vid_compute_mvp(MVP, MV, -1, &mq, mp, &sx.cam, 0);
      glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_TRUE, MVP);
      glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"),  1, GL_TRUE, MV);

      // bind terrain textures:
      glBindTextureUnit( 1, sx.vid.tex_terrain[0]); // colour
      glBindTextureUnit( 2, sx.vid.tex_terrain[1]); // displacement
      glBindTextureUnit( 3, sx.vid.tex_terrain[2]); // detail
      glBindTextureUnit( 4, sx.vid.tex_terrain[3]); // char colour
      glBindTextureUnit( 5, sx.vid.tex_terrain[4]); // char height
      glBindTextureUnit( 6, sx.vid.tex_terrain[5]); // char material
      glBindTextureUnit( 7, sx.vid.tex_terrain[6]); // displacement accel
      glBindTextureUnit( 8, sx.vid.tex_terrain[7]); // char dis accel
      glBindVertexArray(sx.vid.vao_terrain);
      glPatchParameteri(GL_PATCH_VERTICES, 4);
      glDrawElements(GL_PATCHES, sx.vid.terrain_idx_cnt*4, GL_UNSIGNED_INT, 0);
    }

    uint32_t program = sx.vid.program_hero;
    glUseProgram(program);

    // global uniforms:
    // XXX mission -> world?
    // glUniform3f(glGetUniformLocation(program, "u_sun"), sunv[0], sunv[1], sunv[2]);
    glUniform1f(glGetUniformLocation(program, "u_time"), sx.time / 1000.0f);
    glUniform2f(glGetUniformLocation(program, "u_res"), sx.width, sx.height);
    glUniform1i(glGetUniformLocation(program, "u_frame"), sx.time);
    glUniform3f(glGetUniformLocation(program, "u_pos_ws"),
        sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);

    // collect instances
    for(int g=0;g<sx.vid.num_geo;g++)
      sx.vid.geo[g].instances = 0;
    for(int e=0;e<sx.world.num_entities;e++)
      sx_world_render_entity(e);

    // upload mvp matrices
    int offset = 0;
    for(int g=0;g<sx.vid.num_geo;g++)
    {
      if(sx.vid.geo[g].instances > 0)
      {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sx.vid.geo_instance_ssbo);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, 32*sizeof(float)*sx.vid.geo[g].instances, sx.vid.geo[g].instance_mat);
        sx.vid.geo[g].instance_offset = offset / (32*sizeof(float));
        offset += 32*sizeof(float)*sx.vid.geo[g].instances;
      }
    }
    // actually draw
    for(int g=0;g<sx.vid.num_geo;g++)
      if(sx.vid.geo[g].instances > 0)
        sx_vid_render_instanced_geo(g);

    // draw debug force lines
    if(sx.vid.program_debug_line != -1 && sx.vid.debug_line_cnt)
    {
      uint32_t program = sx.vid.program_debug_line;
      glUseProgram(program);

      const sx_entity_t *ent = sx.world.entity + sx.world.player_entity;
      // we have lines almost directly in world space (offset missing, rotation is there)
      float MVP[16], MV[16];
      quat_t mq;
      quat_init_angle(&mq, 0, 1, 0, 0);
      sx_vid_compute_mvp(MVP, MV, -1, &mq, ent->body.c, &sx.cam, 0);
      glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_TRUE, MVP);
      glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"),  1, GL_TRUE, MV);

      glLineWidth(1.5f*sx.width/1024.0f);
      glNamedBufferSubData(sx.vid.vbo_debug_line, 0, sizeof(sx.vid.debug_line_vx), sx.vid.debug_line_vx);
      glBindVertexArray(sx.vid.vao_debug_line);
      glDrawArrays(GL_LINES, 0, sx.vid.debug_line_cnt/6);
    }
#endif

#if 0
    if(sx.vid.program_debug_flow != -1)
    {
      uint32_t program = sx.vid.program_debug_flow;
      glUseProgram(program);

      // we have flow lines in model space (should be world space i guess?)
      float MVP[16], MV[16];
      sx_entity_t *ent = sx.world.entity + sx.world.player_entity;

      // float M[16], MI[16];
      // get_flow_matrix(&ent->body.q, ent->body.c, M);
      // mat4_inv(M, MI);
      // glUniformMatrix4fv(glGetUniformLocation(program, "u_flow"), 1, GL_TRUE, MI);
      // glUniform3f(glGetUniformLocation(program, "u_pos_ws"),
      //     ent->body.c[0], ent->body.c[1], ent->body.c[2]);

      sx_vid_compute_mvp(MVP, MV, -1, &ent->body.q, ent->body.c, &sx.cam, 0);
      glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_TRUE, MVP);
      glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"),  1, GL_TRUE, MV);
      glUniform1f(glGetUniformLocation(program, "u_time"), sx.time / 1000.0f);

      glBindTextureUnit(0, sx.vid.tex_flow[sx.vid.tex_flow_curr]);
      // glBindImageTexture(0, sx.vid.tex_flow[sx.vid.tex_flow_curr], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
      glLineWidth(2.5f*sx.width/1024.0f);
      glBindVertexArray(sx.vid.vao_empty);
      glDrawArrays(GL_LINES, 0, 40000);
    }
#endif

    glDisable(GL_DEPTH_TEST);
    fbo_unbind(sx.vid.raster);
  }

  if(sx.vid.program_draw_env != -1)
  { // deep comp env with geo:
    fbo_bind(sx.vid.fsb);
    // glClearColor(0, 0, 0, 1);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_DEPTH_TEST);
    uint32_t program = sx.vid.program_draw_env;
    glUseProgram(program);

    glUniform1f(glGetUniformLocation(program, "u_hfov"), sx.cam.hfov);
    glUniform2f(glGetUniformLocation(program, "u_res"), sx.width, sx.height);
    glUniform3f(glGetUniformLocation(program, "u_pos"),
        sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);
    glUniform4f(glGetUniformLocation(program, "u_orient"),
        sx.cam.q.x[0], sx.cam.q.x[1],
        sx.cam.q.x[2], sx.cam.q.w);
    glUniform2f(glGetUniformLocation(program, "u_terrain_bounds"),
        sx.world.terrain_low, sx.world.terrain_high);

    glBindTextureUnit(0, old_fbo->tex_color[0]);
    glBindTextureUnit(1, sx.vid.raster->tex_color[0]);
    glBindTextureUnit(2, sx.vid.raster->tex_color[1]);
    glBindTextureUnit(3, sx.vid.raster->tex_depth);
    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    // glDisable(GL_DEPTH_TEST);
    fbo_unbind(sx.vid.fsb);
  }

  // FIXME: diffuse illumination now completely broken
  // need to bind some other buffer, with only environment?
  // glGenerateTextureMipmap(sx.vid.fsb->tex_color[0]);


  // taa:
  // read d->fsb and old_fbo and write to fbo
  fbo_bind(sx.vid.fbo);
  if(sx.vid.program_taa != -1)
  { // do taa from two fbo to one output fbo
    glUseProgram(sx.vid.program_taa);
    GLint res = glGetUniformLocation(sx.vid.program_taa, "u_res");
    glUniform2f(res, sx.vid.fbo->width, sx.vid.fbo->height);
    glBindTextureUnit(0, old_fbo->tex_color[0]);
    glBindTextureUnit(1, sx.vid.fsb->tex_color[0]); // current colour
    glBindTextureUnit(2, sx.vid.fsb->tex_color[1]); // current motion
    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }
  fbo_unbind(sx.vid.fbo);

  // one more pass drawing just the hud frag shader and blitting to render buffer
  if(sx.vid.program_grade != -1)
  {
    glUseProgram(sx.vid.program_grade);
    glUniform3fv(glGetUniformLocation(sx.vid.program_grade, "u_col"), 1, sx.vid.hud.col);
    glUniform3fv(glGetUniformLocation(sx.vid.program_grade, "u_flash_col"), 1, sx.vid.hud.flash_col);
    glUniform1f(glGetUniformLocation(sx.vid.program_grade, "u_hfov"), sx.cam.hfov);
    glUniform1f(glGetUniformLocation(sx.vid.program_grade, "u_vfov"), sx.cam.vfov);
    GLint res = glGetUniformLocation(sx.vid.program_grade, "u_res");
    glUniform2f(res, sx.vid.fbo->width, sx.vid.fbo->height);
    GLint pos = glGetUniformLocation(sx.vid.program_grade, "u_pos");
    glUniform3f(pos,
        sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);
    GLint orient = glGetUniformLocation(sx.vid.program_grade, "u_orient");
    glUniform4f(orient, // object to world
        sx.cam.q.x[0], sx.cam.q.x[1],
        sx.cam.q.x[2], sx.cam.q.w);
    glBindTextureUnit(0, sx.vid.fbo->tex_color[0]);
    // glBindTextureUnit(0, sx.vid.cube[0]->tex_color[0]); // XXX DEBUG cube map
    // glBindTextureUnit(0, sx.vid.fsb->tex_color[0]); // XXX DEBUG no taa
    // glBindTextureUnit(0, sx.vid.raster->tex_color[0]); // XXX DEBUG directly see geo
    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }

  // draw hud lines
  if(sx.vid.program_draw_hud != -1)
  {
    sx_hud_init(&sx.vid.hud, sx.world.player_move);
    glLineWidth(1.5f*sx.width/1024.0f);
    glUseProgram(sx.vid.program_draw_hud);
    glUniform3fv(glGetUniformLocation(sx.vid.program_draw_hud, "u_col"), 1, sx.vid.hud.col);
    glUniform3fv(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_col"), 1, sx.vid.hud.flash_col);
    glUniform1ui(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_beg"), sx.vid.hud.flash_beg);
    if(((int)(sx.time/500.0f))&1)
      glUniform1ui(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_end"), sx.vid.hud.flash_end);
    else
      glUniform1ui(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_end"), sx.vid.hud.flash_beg);
    glNamedBufferSubData(sx.vid.vbo_hud, 0, sizeof(sx.vid.hud.lines), sx.vid.hud.lines);
    glBindVertexArray(sx.vid.vao_hud);
    glDrawArrays(GL_LINES, 0, sx.vid.hud.num_lines);
  }

  // draw hud text
  if(sx.vid.program_hud_text != -1)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(sx.vid.program_hud_text);
    glUniform3fv(glGetUniformLocation(sx.vid.program_hud_text, "u_col"), 1, sx.vid.hud.col);
    glUniform3fv(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_col"), 1, sx.vid.hud.flash_col);
    glUniform1ui(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_beg"), sx.vid.hud.flash_text_beg*4);
    if(((int)(sx.time/500.0f))&1)
      glUniform1ui(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_end"), sx.vid.hud.flash_text_end*4);
    else
      glUniform1ui(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_end"), sx.vid.hud.flash_text_beg*4);
    glNamedBufferSubData(sx.vid.vbo_hud_text[0], 0, sizeof(float)*sx.vid.hud_text_chars*8, sx.vid.hud_text_vx);
    glNamedBufferSubData(sx.vid.vbo_hud_text[1], 0, sizeof(float)*sx.vid.hud_text_chars*8, sx.vid.hud_text_uv);
    // glNamedBufferSubData(sx.vid.vbo_hud_text[2], 0, sx.vid.hud_text_chars*6, sx.vid.hud_text_id);
    glBindTextureUnit(0, sx.vid.hud_text_font);
    glBindVertexArray(sx.vid.vao_hud_text);
    glDrawElements(GL_TRIANGLES, sx.vid.hud_text_chars*6, GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
  }

  sx.vid.fbo = old_fbo;
  SDL_GL_SwapWindow(sx.vid.window);
}

void sx_vid_render_frame()
{
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  sx.vid.geo_instance_cnt = 0;

  fbo_t *old_fbo = sx.vid.fbo == sx.vid.fbo0 ? sx.vid.fbo1 : sx.vid.fbo0;

  // half angle of horizontal field of view. pi/2 means 180 degrees total
  // and is the max we can get with only two cubemap sides

  // rasterise geo before ray tracing
  if(sx.vid.program_hero != -1)
  {
    // TODO: get from world
    // float sunv[] = {-0.624695,0.468521,-0.624695};
    // normalise(sunv);
    // quat_transform_inv(&sx.cam.q, sunv); // sun in camera space

    // turns out we only need L and R cube sides
    for(int k=0;k<2;k++)
    {
      sx.vid.cube_side = k;
      fbo_bind(sx.vid.cube[k]);
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      // glEnable(GL_BLEND);
      // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      // glBlendFunc(GL_ONE, GL_ONE);
      // glBlendFunc(GL_ZERO, GL_SRC_COLOR);

      // render terrain first and hope for early z out?
      // XXX DEBUG draw tessellated terrain
      if(sx.vid.program_terrain != -1)
      {
        uint32_t program = sx.vid.program_terrain;
        glUseProgram(program);
        glUniform1f(glGetUniformLocation(program, "u_time"), sx.time / 1000.0f);
        glUniform1f(glGetUniformLocation(program, "u_lod"), 1);
        glUniform2f(glGetUniformLocation(program, "u_res"),
            sx.vid.cube[k]->width, sx.vid.cube[k]->height);
        glUniform1i(glGetUniformLocation(program, "u_frame"), sx.time);
        glUniform3f(glGetUniformLocation(program, "u_pos_ws"),
            sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);
        glUniform2f(glGetUniformLocation(program, "u_terrain_bounds"),
            sx.world.terrain_low, sx.world.terrain_high);

        // terrain comes in world space but centered at camera position
        float MVP[16], MV[16];
        quat_t mq;
        quat_init_angle(&mq, 0, 1, 0, 0);
        float mp[] = {sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]};
        sx_vid_compute_mvp(MVP, MV, k, &mq, mp, &sx.cam, 0);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_TRUE, MVP);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"),  1, GL_TRUE, MV);

        // bind terrain textures:
        glBindTextureUnit( 1, sx.vid.tex_terrain[0]); // colour
        glBindTextureUnit( 2, sx.vid.tex_terrain[1]); // displacement
        glBindTextureUnit( 3, sx.vid.tex_terrain[2]); // detail
        glBindTextureUnit( 4, sx.vid.tex_terrain[3]); // char colour
        glBindTextureUnit( 5, sx.vid.tex_terrain[4]); // char height
        glBindTextureUnit( 6, sx.vid.tex_terrain[5]); // char material
        glBindTextureUnit( 7, sx.vid.tex_terrain[6]); // displacement accel
        glBindTextureUnit( 8, sx.vid.tex_terrain[7]); // char dis accel
        glBindVertexArray(sx.vid.vao_terrain);
        glDrawElements(GL_PATCHES, sx.vid.terrain_idx_cnt*4, GL_UNSIGNED_INT, 0);
      }

      uint32_t program = sx.vid.program_hero;
      glUseProgram(program);

      // global uniforms:
      // XXX mission -> world?
      // glUniform3f(glGetUniformLocation(program, "u_sun"), sunv[0], sunv[1], sunv[2]);
      glUniform1f(glGetUniformLocation(program, "u_time"), sx.time / 1000.0f);
      glUniform2f(glGetUniformLocation(program, "u_res"),
          sx.vid.cube[k]->width, sx.vid.cube[k]->height);
      glUniform1i(glGetUniformLocation(program, "u_frame"), sx.time);
      glUniform3f(glGetUniformLocation(program, "u_pos_ws"),
          sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);

      // collect instances
      for(int g=0;g<sx.vid.num_geo;g++)
        sx.vid.geo[g].instances = 0;
      for(int e=0;e<sx.world.num_entities;e++)
        sx_world_render_entity(e);

      // upload mvp matrices
      int offset = 0;
      for(int g=0;g<sx.vid.num_geo;g++)
      {
        if(sx.vid.geo[g].instances > 0)
        {
          glBindBuffer(GL_SHADER_STORAGE_BUFFER, sx.vid.geo_instance_ssbo);
          glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, 32*sizeof(float)*sx.vid.geo[g].instances, sx.vid.geo[g].instance_mat);
          sx.vid.geo[g].instance_offset = offset / (32*sizeof(float));
          offset += 32*sizeof(float)*sx.vid.geo[g].instances;
        }
      }
      // actually draw
      for(int g=0;g<sx.vid.num_geo;g++)
        if(sx.vid.geo[g].instances > 0)
          sx_vid_render_instanced_geo(g);

      // draw debug force lines
      if(sx.vid.program_debug_line != -1 && sx.vid.debug_line_cnt)
      {
        uint32_t program = sx.vid.program_debug_line;
        glUseProgram(program);

        // we have lines almost directly in world space
        float MVP[16], MV[16];
        quat_t mq;
        quat_init_angle(&mq, 0, 1, 0, 0);
        sx_entity_t *ent = sx.world.entity + sx.world.player_entity;
        float mp[] = {ent->body.c[0], ent->body.c[1], ent->body.c[2]};
        sx_vid_compute_mvp(MVP, MV, k, &mq, mp, &sx.cam, 0);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_TRUE, MVP);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"),  1, GL_TRUE, MV);

        glLineWidth(1.5f*sx.width/1024.0f);
        glNamedBufferSubData(sx.vid.vbo_debug_line, 0, sizeof(sx.vid.debug_line_vx), sx.vid.debug_line_vx);
        glBindVertexArray(sx.vid.vao_debug_line);
        glDrawArrays(GL_LINES, 0, sx.vid.debug_line_cnt/6);
      }

      glDisable(GL_DEPTH_TEST);
      // glDisable(GL_BLEND);
      fbo_unbind(sx.vid.cube[k]);
    }
  }

  // now unstretch geo to fisheye buffer
  fbo_bind(sx.vid.raster);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if(sx.vid.program_blit_texture != -1)
  {
    glUseProgram(sx.vid.program_blit_texture);
    glUniform1f(glGetUniformLocation(sx.vid.program_blit_texture, "u_lod"), 1);// XXXrt.rt_lod);
    glUniform1f(glGetUniformLocation(sx.vid.program_blit_texture, "u_hfov"), sx.cam.hfov);
    glUniform1f(glGetUniformLocation(sx.vid.program_blit_texture, "u_vfov"), sx.cam.vfov);
    glUniform1f(glGetUniformLocation(sx.vid.program_blit_texture, "u_time"), sx.time / 1000.0f);
    glUniform3f(glGetUniformLocation(sx.vid.program_blit_texture, "u_pos"),
        sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);
    // object to world
    glUniform4f(glGetUniformLocation(sx.vid.program_blit_texture, "u_orient"),
        sx.cam.q.x[0], sx.cam.q.x[1],
        sx.cam.q.x[2], sx.cam.q.w);
    glBindTextureUnit(0, sx.vid.cube[0]->tex_color[0]);
    glBindTextureUnit(1, sx.vid.cube[0]->tex_color[1]);
    glBindTextureUnit(2, sx.vid.cube[1]->tex_color[0]);
    glBindTextureUnit(3, sx.vid.cube[1]->tex_color[1]);
    glBindTextureUnit(4, sx.vid.cube[0]->tex_depth);
    glBindTextureUnit(5, sx.vid.cube[1]->tex_depth);

    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }
  fbo_unbind(sx.vid.raster);

  if(sx.vid.program_draw_texture != -1)
  { // ray trace landscape and do deep comp with geo:
    fbo_bind(sx.vid.fsb);
    // glClearColor(0, 0, 0, 1);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_DEPTH_TEST);
    glUseProgram(sx.vid.program_draw_texture);
    glUniform1f(glGetUniformLocation(sx.vid.program_draw_texture, "u_hfov"), sx.cam.hfov);
    glUniform1f(glGetUniformLocation(sx.vid.program_draw_texture, "u_vfov"), sx.cam.vfov);
    glUniform2f(glGetUniformLocation(sx.vid.program_draw_texture, "u_res"),
        sx.vid.fbo->width, sx.vid.fbo->height);
    glUniform2f(glGetUniformLocation(sx.vid.program_draw_texture, "u_terrain_bounds"),
        sx.world.terrain_low, sx.world.terrain_high);
    glUniform1f(glGetUniformLocation(sx.vid.program_draw_texture, "u_lod"),
        1);// XXX rt.rt_lod);
    glUniform1f(glGetUniformLocation(sx.vid.program_draw_texture, "u_time"),
        sx.time / 1000.0f);
    glUniform3f(glGetUniformLocation(sx.vid.program_draw_texture, "u_pos"),
        sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);
    glUniform4f(glGetUniformLocation(sx.vid.program_draw_texture, "u_orient"),
        sx.cam.q.x[0], sx.cam.q.x[1], sx.cam.q.x[2], sx.cam.q.w);
    glUniform3f(glGetUniformLocation(sx.vid.program_draw_texture, "u_old_pos"),
        sx.cam.prev_x[0], sx.cam.prev_x[1], sx.cam.prev_x[2]);
    glUniform4f(glGetUniformLocation(sx.vid.program_draw_texture, "u_old_orient"),
        sx.cam.prev_q.x[0], sx.cam.prev_q.x[1],
        sx.cam.prev_q.x[2], sx.cam.prev_q.w);

    glBindTextureUnit( 0, old_fbo->tex_color[0]);
    glBindTextureUnit( 1, sx.vid.tex_terrain[0]); // colour
    glBindTextureUnit( 2, sx.vid.tex_terrain[1]); // displacement
    glBindTextureUnit( 3, sx.vid.tex_terrain[2]); // detail
    glBindTextureUnit( 4, sx.vid.tex_terrain[3]); // char colour
    glBindTextureUnit( 5, sx.vid.tex_terrain[4]); // char height
    glBindTextureUnit( 6, sx.vid.tex_terrain[5]); // char material
    glBindTextureUnit( 7, sx.vid.tex_terrain[6]); // displacement accel
    glBindTextureUnit( 8, sx.vid.tex_terrain[7]); // char dis accel
    glBindTextureUnit( 9, sx.vid.raster->tex_color[0]);
    glBindTextureUnit(10, sx.vid.raster->tex_color[1]);
    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    // glDisable(GL_DEPTH_TEST);
    fbo_unbind(sx.vid.fsb);
  }

  // FIXME: diffuse illumination now completely broken
  // need to bind some other buffer, with only environment?
  // glGenerateTextureMipmap(sx.vid.fsb->tex_color[0]);


  // taa:
  // read d->fsb and old_fbo and write to fbo
  fbo_bind(sx.vid.fbo);
  if(sx.vid.program_taa != -1)
  { // do taa from two fbo to one output fbo
    glUseProgram(sx.vid.program_taa);
    GLint res = glGetUniformLocation(sx.vid.program_taa, "u_res");
    glUniform2f(res, sx.vid.fbo->width, sx.vid.fbo->height);
    glBindTextureUnit(0, old_fbo->tex_color[0]);
    glBindTextureUnit(1, sx.vid.fsb->tex_color[0]); // current colour
    glBindTextureUnit(2, sx.vid.fsb->tex_color[1]); // current motion
    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }
  fbo_unbind(sx.vid.fbo);

  // one more pass drawing just the hud frag shader and blitting to render buffer
  if(sx.vid.program_grade != -1)
  {
    glUseProgram(sx.vid.program_grade);
    glUniform3fv(glGetUniformLocation(sx.vid.program_grade, "u_col"), 1, sx.vid.hud.col);
    glUniform3fv(glGetUniformLocation(sx.vid.program_grade, "u_flash_col"), 1, sx.vid.hud.flash_col);
    glUniform1f(glGetUniformLocation(sx.vid.program_grade, "u_hfov"), sx.cam.hfov);
    glUniform1f(glGetUniformLocation(sx.vid.program_grade, "u_vfov"), sx.cam.vfov);
    GLint res = glGetUniformLocation(sx.vid.program_grade, "u_res");
    glUniform2f(res, sx.vid.fbo->width, sx.vid.fbo->height);
    GLint pos = glGetUniformLocation(sx.vid.program_grade, "u_pos");
    glUniform3f(pos,
        sx.cam.x[0], sx.cam.x[1], sx.cam.x[2]);
    GLint orient = glGetUniformLocation(sx.vid.program_grade, "u_orient");
    glUniform4f(orient, // object to world
        sx.cam.q.x[0], sx.cam.q.x[1],
        sx.cam.q.x[2], sx.cam.q.w);
    glBindTextureUnit(0, sx.vid.fbo->tex_color[0]);
    // glBindTextureUnit(0, sx.vid.cube[0]->tex_color[0]); // XXX DEBUG cube map
    // glBindTextureUnit(0, sx.vid.fsb->tex_color[0]); // XXX DEBUG no taa
    // glBindTextureUnit(0, sx.vid.raster->tex_color[0]); // XXX DEBUG directly see geo
    glBindVertexArray(sx.vid.vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }

  // draw hud lines
  if(sx.vid.program_draw_hud != -1)
  {
    sx_hud_init(&sx.vid.hud, sx.world.player_move);
    glLineWidth(1.5f*sx.width/1024.0f);
    glUseProgram(sx.vid.program_draw_hud);
    glUniform3fv(glGetUniformLocation(sx.vid.program_draw_hud, "u_col"), 1, sx.vid.hud.col);
    glUniform3fv(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_col"), 1, sx.vid.hud.flash_col);
    glUniform1ui(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_beg"), sx.vid.hud.flash_beg);
    if(((int)(sx.time/500.0f))&1)
      glUniform1ui(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_end"), sx.vid.hud.flash_end);
    else
      glUniform1ui(glGetUniformLocation(sx.vid.program_draw_hud, "u_flash_end"), sx.vid.hud.flash_beg);
    glNamedBufferSubData(sx.vid.vbo_hud, 0, sizeof(sx.vid.hud.lines), sx.vid.hud.lines);
    glBindVertexArray(sx.vid.vao_hud);
    glDrawArrays(GL_LINES, 0, sx.vid.hud.num_lines);
  }

  // draw hud text
  if(sx.vid.program_hud_text != -1)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(sx.vid.program_hud_text);
    glUniform3fv(glGetUniformLocation(sx.vid.program_hud_text, "u_col"), 1, sx.vid.hud.col);
    glUniform3fv(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_col"), 1, sx.vid.hud.flash_col);
    glUniform1ui(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_beg"), sx.vid.hud.flash_text_beg*4);
    if(((int)(sx.time/500.0f))&1)
      glUniform1ui(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_end"), sx.vid.hud.flash_text_end*4);
    else
      glUniform1ui(glGetUniformLocation(sx.vid.program_hud_text, "u_flash_end"), sx.vid.hud.flash_text_beg*4);
    glNamedBufferSubData(sx.vid.vbo_hud_text[0], 0, sizeof(float)*sx.vid.hud_text_chars*8, sx.vid.hud_text_vx);
    glNamedBufferSubData(sx.vid.vbo_hud_text[1], 0, sizeof(float)*sx.vid.hud_text_chars*8, sx.vid.hud_text_uv);
    // glNamedBufferSubData(sx.vid.vbo_hud_text[2], 0, sx.vid.hud_text_chars*6, sx.vid.hud_text_id);
    glBindTextureUnit(0, sx.vid.hud_text_font);
    glBindVertexArray(sx.vid.vao_hud_text);
    glDrawElements(GL_TRIANGLES, sx.vid.hud_text_chars*6, GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
  }

  sx.vid.fbo = old_fbo;
  SDL_GL_SwapWindow(sx.vid.window);
}

int sx_vid_handle_input()
{
  SDL_Event event;
  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      // esc button is pressed
      case SDL_QUIT:
        // set flag to exit, return something?
        return 1;
      case SDL_MOUSEBUTTONDOWN:
        break;
      case SDL_MOUSEBUTTONUP:
        break;
      case SDL_MOUSEMOTION: // XXX DEBUG just to see something moving at all
        if(event.motion.state & SDL_BUTTON_LMASK)
        {
          sx.cam.x[0] -= 2*event.motion.xrel;
          sx.cam.x[2] += 2*event.motion.yrel;
          sx.cam.x[1] = sx_world_get_height(sx.cam.x)+10.0f;
          float off[3] = {0, 0.0, 0};
          sx_camera_target(&sx.cam, sx.cam.x, &sx.cam.q, off, 1.0f, 1.0f);
        }
        break;
      case SDL_KEYDOWN:
      switch(event.key.keysym.sym)
      {
        case SDLK_ESCAPE:
          return 1;
        case SDLK_r:
          fprintf(stderr, "[vid] recompiling shaders\n");
          recompile();
          break;
        case SDLK_F1:
          sx.cam.mode = s_cam_inside_cockpit;
          break;
        case SDLK_F2:
          sx.cam.mode = s_cam_inside_no_cockpit;
          break;
        case SDLK_F3:
          sx.cam.mode = s_cam_left;
          break;
        case SDLK_F4:
          sx.cam.mode = s_cam_right;
          break;
        case SDLK_F5:
          sx.cam.mode = s_cam_flyby;
          break;
        case SDLK_F6:
          sx.cam.mode = s_cam_homing;
          break;
        case SDLK_F7:
          sx.cam.hfov *= 1.1f;
          sx.cam.vfov = sx.cam.hfov * sx.height/(float)sx.width;
          break;
        case SDLK_F8:
          sx.cam.hfov /= 1.1f;
          sx.cam.vfov = sx.cam.hfov * sx.height/(float)sx.width;
          break;
        case SDLK_d: // down
          {
            float pos[3] = {sx.cam.x[0], 0.0, sx.cam.x[2]};
            pos[1] = sx_world_get_height(pos);
            float off[3] = {0, 1, 0};
            quat_t q;
            quat_init_angle(&q, 0, 1, 0, 0);
            sx_camera_target(&sx.cam, pos, &q, off, 0.02f, 0.02f);
          }
          break;
        case SDLK_t: // top
          {
            float pos[3] = {sx.cam.x[0], 2050.0, sx.cam.x[2]};
            float off[3] = {0, 0, 0};
            quat_t q;
            quat_init_angle(&q, M_PI/2.0f, 1, 0, 0);
            sx_camera_target(&sx.cam, pos, &q, off, 1.0f, 1.0f);
            sx_camera_move(&sx.cam, 0.01f);
            sx_camera_target(&sx.cam, pos, &q, off, 0.02f, 0.02f);
          }
          break;
        case SDLK_1:
          sx_heli_control_collective(sx.world.player_move, 0.1);
          break;
        case SDLK_2:
          sx_heli_control_collective(sx.world.player_move, 0.2);
          break;
        case SDLK_3:
          sx_heli_control_collective(sx.world.player_move, 0.3);
          break;
        case SDLK_4:
          sx_heli_control_collective(sx.world.player_move, 0.4);
          break;
        case SDLK_5:
          sx_heli_control_collective(sx.world.player_move, 0.5);
          break;
        case SDLK_6:
          sx_heli_control_collective(sx.world.player_move, 0.6);
          break;
        case SDLK_7:
          sx_heli_control_collective(sx.world.player_move, 0.7);
          break;
        case SDLK_8:
          sx_heli_control_collective(sx.world.player_move, 0.8);
          break;
        case SDLK_9:
          sx_heli_control_collective(sx.world.player_move, 0.9);
          break;
        case SDLK_0:
          sx_heli_control_collective(sx.world.player_move, 1.0);
          break;
        case SDLK_UP:
          sx_heli_control_cyclic_z(sx.world.player_move, 1);
          break;
        case SDLK_DOWN:
          sx_heli_control_cyclic_z(sx.world.player_move, -1);
          break;
        case SDLK_RIGHT:
          sx_heli_control_cyclic_x(sx.world.player_move, 1);
          break;
        case SDLK_LEFT:
          sx_heli_control_cyclic_x(sx.world.player_move, -1);
          break;
        case SDLK_a:
          sx_heli_control_tail(sx.world.player_move, -1);
          break;
        case SDLK_o:
          sx_heli_control_tail(sx.world.player_move, 1);
          break;
        case SDLK_g:
          sx_heli_control_gear(sx.world.player_move);
          break;
        case SDLK_f:
          sx_heli_control_flap(sx.world.player_move);
          break;
      }
      break;
      case SDL_KEYUP:
      switch(event.key.keysym.sym)
      {
        case SDLK_UP:
        case SDLK_DOWN:
          sx_heli_control_cyclic_z(sx.world.player_move, 0);
        case SDLK_RIGHT:
        case SDLK_LEFT:
          sx_heli_control_cyclic_x(sx.world.player_move, 0);
          break;
        case SDLK_a:
        case SDLK_o:
          sx_heli_control_tail(sx.world.player_move, 0);
          break;
      }
      break;
      case SDL_JOYAXISMOTION:
      switch(event.jaxis.axis)
      {
        case 0:
          sx_heli_control_cyclic_x(sx.world.player_move, event.jaxis.value/(float)0x7fff);
          break;
        case 1:
          sx_heli_control_cyclic_z(sx.world.player_move, -event.jaxis.value/(float)0x7fff);
          break;
        case 2:
          {
          // // remap to avoid dead zone on retarded thrustmaster T.Flight Hotas X
          float t = event.jaxis.value/(float)0x7fff;
          // if(t > 0) t *= 0.5f; // overdrive
          sx_heli_control_collective(sx.world.player_move, 0.5f + 0.5*t);
          break;
          }
        case 3:
          sx_heli_control_tail(sx.world.player_move, event.jaxis.value/(float)0x7fff);
          break;
        case 4:
          sx.cam.angle_right = event.jaxis.value/(float)0x7fff;
          break;
      }
      break;
      case SDL_JOYHATMOTION:
      switch(event.jhat.hat)
      {
        case 0:
          if(event.jhat.value == SDL_HAT_CENTERED)
            sx.cam.angle_right = sx.cam.angle_down = 0.0f;
          else if(event.jhat.value == SDL_HAT_LEFT)
            sx.cam.angle_right = -1.0f;
          else if(event.jhat.value == SDL_HAT_RIGHT)
            sx.cam.angle_right = 1.0f;
          else if(event.jhat.value == SDL_HAT_UP)
            sx.cam.angle_down = -1.0f;
          else if(event.jhat.value == SDL_HAT_DOWN)
            sx.cam.angle_down = 1.0f;
          break;
      }
      break;
      case SDL_JOYBUTTONDOWN:
      switch(event.jbutton.button)
      {
        case 0: // pretend we have a cannon:
          sx_sound_play(sx.assets.sound+sx.mission.snd_cannon, -1);
          break;
        case 3:
          sx_heli_control_tail(sx.world.player_move, -1);
          break;
        case 4:
          sx_heli_control_tail(sx.world.player_move, 1);
          break;
      }
      break;
      case SDL_JOYBUTTONUP:
      switch(event.jbutton.button)
      {
        case 3:
        case 4:
          sx_heli_control_tail(sx.world.player_move, 0);
          break;
      }
      break;
    }
  }
  return 0;
}

void
sx_vid_clear_debug_line()
{
  sx.vid.debug_line_cnt = 0;
}

void
sx_vid_add_debug_line(
    const float *v0,
    const float *v1)
{
  if(sx.vid.debug_line_cnt >= sizeof(sx.vid.debug_line_vx)/sizeof(sx.vid.debug_line_vx[0]) - 6)
    return;
  sx.vid.debug_line_vx[sx.vid.debug_line_cnt++] = v0[0];
  sx.vid.debug_line_vx[sx.vid.debug_line_cnt++] = v0[1];
  sx.vid.debug_line_vx[sx.vid.debug_line_cnt++] = v0[2];
  sx.vid.debug_line_vx[sx.vid.debug_line_cnt++] = v1[0];
  sx.vid.debug_line_vx[sx.vid.debug_line_cnt++] = v1[1];
  sx.vid.debug_line_vx[sx.vid.debug_line_cnt++] = v1[2];
}
