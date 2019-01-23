#pragma once

#include <GL/glew.h>
#include <assert.h>

#define FBO_MAX_COLOR_ATTACHMENTS 8

typedef struct fbo_init_t
{
	unsigned int color[FBO_MAX_COLOR_ATTACHMENTS];
	unsigned int depth;
	unsigned int rb;
	unsigned int samples;
	int width, height;
	const char *label;
	int depth_samples, color_samples;
	int color_mip;
	int depth_mip;
}
fbo_init_t;

#if 0
	FBOInit(int w, int h) : width(w), height(h) {}

	FBOInit & attach_color(int idx, GLenum type) {
		color[idx] = type;
		return *this;
	}

	FBOInit & attach_depth(GLenum type) {
		depth = type;
		return *this;
	}

	FBOInit & attach_rb(GLenum type) {
		rb = type;
		return *this;
	}

	FBOInit & num_samples(int s) {
		samples = s;
		return *this;
	}

	FBOInit & attach_label(const char *l) {
		label = l;
		return *this;
	}

	FBOInit & mixed_samples(int num_color_samples, int num_depth_samples) {
		color_samples = num_color_samples;
		depth_samples = num_depth_samples;
		assert(color_samples > 0);
		assert(depth_samples > 1);
		return *this;
	}

	FBOInit & use_color_mip() {
		color_mip = true;
		return *this;
	}

	FBOInit & use_depth_mip() {
		depth_mip = true;
		return *this;
	}
};
#endif

typedef struct fbo_t
{
	int width, height;
	unsigned int fbo;
	unsigned int tex_color[FBO_MAX_COLOR_ATTACHMENTS];
	unsigned int rb;
	unsigned int tex_depth;
	int old_view[4];
	int old_fb;
#ifndef NDEBUG
	int is_bound;
#endif
	int free_mask;
}
fbo_t;

static inline void
fbo_bind(fbo_t *fbo)
{
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo->old_fb);
  glGetIntegerv(GL_VIEWPORT, fbo->old_view);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
  glViewport(0, 0, fbo->width, fbo->height);
#ifndef NDEBUG
  assert(!fbo->is_bound);
  fbo->is_bound = 1;
  assert(fbo->old_fb != fbo->fbo);
#endif
}

static inline void
fbo_unbind(fbo_t *fbo)
{
#ifndef NDEBUG
  assert(fbo->is_bound);
  fbo->is_bound = 0;
#endif
  glBindFramebuffer(GL_FRAMEBUFFER, fbo->old_fb);
  glViewport(fbo->old_view[0], fbo->old_view[1], fbo->old_view[2], fbo->old_view[3]);
}

static inline void
fbo_init_ms(fbo_t *fb, const fbo_init_t *fbo_init);
static inline void
fbo_init_std(fbo_t *fb, const fbo_init_t *fbo_init);
static inline void
fbo_init_mixed_samples(fbo_t *fb, const fbo_init_t *fbo_init);
static inline void
fbo_attach_labels(fbo_t *fb, const fbo_init_t *fbo_init);
static inline void
fbo_attach_rb(fbo_t *fb, GLenum type_, int rb_);
static inline void
fbo_attach_texture(fbo_t *fb, int idx_, int tex_);
static inline void
fb_attach_depth_tex(fbo_t *fb, GLenum target, int tex_);
static inline void
fbo_check_framebuffer_complete(fbo_t *fb);


static inline fbo_t* fbo_init(fbo_init_t *fi)
{
  fbo_t *f = malloc(sizeof(fbo_t));
  memset(f, 0, sizeof(*f));
  f->free_mask = ~0;
  f->width  = fi->width;
  f->height = fi->height;

  if(fi->depth_samples > 0 && fi->color_samples > 0)
    fbo_init_mixed_samples(f, fi);
  else if(fi->samples > 0)
    fbo_init_ms(f, fi);
  else
    fbo_init_std(f, fi);

  if(fi->label)
    fbo_attach_labels(f, fi);

  return f;
}

static inline void fbo_cleanup(fbo_t *fb)
{
#ifndef NDEBUG
	assert(!fb->is_bound);
#endif
	for(int i = 0; i < FBO_MAX_COLOR_ATTACHMENTS; i++) {
		if(fb->free_mask & (1 << i)) {
			glDeleteTextures(1, &fb->tex_color[i]);
		}
	}
	if(fb->free_mask & (1 << FBO_MAX_COLOR_ATTACHMENTS)) {
		glDeleteRenderbuffers(1, &fb->rb);
	}
	if(fb->free_mask & (1 << (FBO_MAX_COLOR_ATTACHMENTS + 1))) {
		glDeleteTextures(1, &fb->tex_depth);
	}
	glDeleteFramebuffers(1, &fb->fbo);
  free(fb);
}


static inline GLuint
fbo_gen_renderbuffer_ms(GLenum internal_format, int w, int h, int samples)
{
	GLuint rb;
	glGenRenderbuffers(1, &rb);
	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
			internal_format, w, h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return rb;
}

static inline GLuint
fbo_gen_texture_ms(GLenum internal_format, int w, int h, int samples)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
#ifndef RENDER_UTILS_GLES
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples,
			internal_format, w, h, GL_TRUE);
#endif
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	return tex;
}

static inline GLuint
fbo_gen_renderbuffer(GLenum internal_format, int w, int h)
{
	GLuint rb;
	glGenRenderbuffers(1, &rb);
	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glRenderbufferStorage(GL_RENDERBUFFER, internal_format, w, h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return rb;
}

static inline GLuint
fbo_gen_texture(GLenum internal_format, int w, int h)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexStorage2D(GL_TEXTURE_2D, 1, internal_format, w, h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

static inline GLuint
fbo_gen_texture_with_mip(GLenum internal_format, int w, int h)
{
	int num_levels = 0;
	for(int i = w > h ? w : h; i > 0; i /= 2)
		num_levels++;

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexStorage2D(GL_TEXTURE_2D, num_levels, internal_format, w, h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

static inline GLuint
fbo_gen_fbo()
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	return fbo;
}

static inline void
fbo_check_framebuffer_complete(fbo_t *fb)
{
	GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
    fprintf(stderr, "error setting up fbo: %d\n", status);
			 // GLUtil::map_gl_enum_to_string(status));
}

static inline void
fbo_init_ms(fbo_t *fb, const fbo_init_t *fbo_init)
{
	int w = fb->width, h = fb->height;
	glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo = fbo_gen_fbo());

	GLenum active_attachments[FBO_MAX_COLOR_ATTACHMENTS];
	GLenum f;
	for(int i = 0; i < FBO_MAX_COLOR_ATTACHMENTS; i++) {
		if((f = fbo_init->color[i])) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
					GL_TEXTURE_2D_MULTISAMPLE,
					fb->tex_color[i] = fbo_gen_texture_ms(f, w, h, fbo_init->samples), 0);
			active_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		else {
			active_attachments[i] = GL_NONE;
		}
	}
	glDrawBuffers(FBO_MAX_COLOR_ATTACHMENTS, active_attachments);

	if((f = fbo_init->rb)) {
		GLenum attach = GL_DEPTH_ATTACHMENT;
		if(f == GL_DEPTH24_STENCIL8 || f == GL_DEPTH32F_STENCIL8) {
			attach = GL_DEPTH_STENCIL_ATTACHMENT;
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach, GL_RENDERBUFFER,
				fb->rb = fbo_gen_renderbuffer_ms(f, w, h, fbo_init->samples));
	}

	if((f = fbo_init->depth)) {
		fb->tex_depth = fbo_gen_texture_ms(f, w, h, fbo_init->samples);
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
#ifndef RENDER_UTILS_GLES
		glNamedFramebufferTexture(fb->fbo, GL_DEPTH_ATTACHMENT, fb->tex_depth, 0);
		glTextureParameterfv(fb->tex_depth, GL_TEXTURE_BORDER_COLOR, l_ClampColor);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_BORDER);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_BORDER);
#else
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, fb->tex_depth, 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fb->tex_depth);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE,     GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC,     GL_LEQUAL);
		glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR_EXT, l_ClampColor);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#endif
	}

	fbo_check_framebuffer_complete(fb);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static inline void
fbo_init_std(fbo_t *fb, const fbo_init_t *fbo_init)
{
	int w = fb->width, h = fb->height;
	glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo = fbo_gen_fbo());

	GLenum active_attachments[FBO_MAX_COLOR_ATTACHMENTS];

	GLenum f;
	for(int i = 0; i < FBO_MAX_COLOR_ATTACHMENTS; i++) {
		if((f = fbo_init->color[i])) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
					GL_TEXTURE_2D, fb->tex_color[i] = (fbo_init->color_mip ? fbo_gen_texture_with_mip : fbo_gen_texture)(f, w, h), 0);
			active_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		else {
			active_attachments[i] = GL_NONE;
		}
	}
	glDrawBuffers(FBO_MAX_COLOR_ATTACHMENTS, active_attachments);

	if((f = fbo_init->rb)) {
		GLenum attach = GL_DEPTH_ATTACHMENT;
		if(f == GL_DEPTH24_STENCIL8 || f == GL_DEPTH32F_STENCIL8) {
			attach = GL_DEPTH_STENCIL_ATTACHMENT;
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach, GL_RENDERBUFFER,
				fb->rb = fbo_gen_renderbuffer(f, w, h));
	}

	if((f = fbo_init->depth)) {

		fb->tex_depth = (fbo_init->depth_mip ? fbo_gen_texture_with_mip : fbo_gen_texture)(f, w, h);
#ifndef RENDER_UTILS_GLES
		glNamedFramebufferTexture(fb->fbo, GL_DEPTH_ATTACHMENT, fb->tex_depth, 0);
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glTextureParameterfv(fb->tex_depth, GL_TEXTURE_BORDER_COLOR, l_ClampColor);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_BORDER);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_BORDER);
#else
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->tex_depth, 0);
		glBindTexture(GL_TEXTURE_2D, fb->tex_depth);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,     GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,     GL_LEQUAL);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_EXT, l_ClampColor);
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
	}

#ifndef RENDER_UTILS_GLES
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, fb->width);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, fb->height);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 1);
#endif

	fbo_check_framebuffer_complete(fb);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static inline void
fbo_init_mixed_samples(fbo_t *fb, const fbo_init_t *fbo_init)
{
	assert(fbo_init->depth_samples > fbo_init->color_samples);
	assert(fbo_init->depth_samples > 1);
	assert(fbo_init->color_samples > 0);
	assert(!fbo_init->color_mip);

	int w = fb->width, h = fb->height;
	glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo = fbo_gen_fbo());

	GLenum active_attachments[FBO_MAX_COLOR_ATTACHMENTS];

	int nc = fbo_init->color_samples;
	int nd = fbo_init->depth_samples;

	GLenum f;
	for(int i = 0; i < FBO_MAX_COLOR_ATTACHMENTS; i++) {
		if((f = fbo_init->color[i])) {
//			if(nc > 1) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
						GL_TEXTURE_2D_MULTISAMPLE,
						fb->tex_color[i] = fbo_gen_texture_ms(f, w, h, nc), 0);
//			}
//			else {
//				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
//						tex_color[i] = gen_texture(f, w, h), 0);
//			}
			active_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		else {
			active_attachments[i] = GL_NONE;
		}
	}
	glDrawBuffers(FBO_MAX_COLOR_ATTACHMENTS, active_attachments);

	if((f = fbo_init->rb)) {
		GLenum attach = GL_DEPTH_ATTACHMENT;
		if(f == GL_DEPTH24_STENCIL8 || f == GL_DEPTH32F_STENCIL8) {
			attach = GL_DEPTH_STENCIL_ATTACHMENT;
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach, GL_RENDERBUFFER,
				fb->rb = fbo_gen_renderbuffer_ms(f, w, h, nd));
	}

	if((f = fbo_init->depth)) {
		fb->tex_depth = fbo_gen_texture(f, w, h);
#ifndef RENDER_UTILS_GLES
		glNamedFramebufferTexture(fb->fbo, GL_DEPTH_ATTACHMENT, fb->tex_depth, 0);
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glTextureParameterfv(fb->tex_depth, GL_TEXTURE_BORDER_COLOR, l_ClampColor);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_BORDER);
		glTextureParameteri (fb->tex_depth, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_BORDER);
#else
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->tex_depth, 0);
		glBindTexture(GL_TEXTURE_2D, fb->tex_depth);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,     GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,     GL_LEQUAL);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_EXT, l_ClampColor);
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
	}

#ifndef RENDER_UTILS_GLES
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, fb->width);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, fb->height);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 1);
#endif

	fbo_check_framebuffer_complete(fb);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static inline void
fbo_attach_labels(fbo_t *fb, const fbo_init_t *fbo_init)
{
#ifndef RENDER_UTILS_GLES
	char buf[1024];

	for(int i = 0; i < FBO_MAX_COLOR_ATTACHMENTS; i++) {
		if(fbo_init->color[i]) {
			snprintf(buf, sizeof buf, "%s_tex_col_%02d", fbo_init->label, i);
			glObjectLabel(GL_TEXTURE, fb->tex_color[i], -1, buf);
		}
	}

	if(fbo_init->rb) {
		snprintf(buf, sizeof buf, "%s_rb", fbo_init->label);
		glObjectLabel(GL_RENDERBUFFER, fb->rb, -1, buf);
	}

	if(fbo_init->depth) {
		snprintf(buf, sizeof buf, "%s_tex_depth", fbo_init->label);
		glObjectLabel(GL_TEXTURE, fb->tex_depth, -1, buf);
	}

	glObjectLabel(GL_FRAMEBUFFER, fb->fbo, -1, fbo_init->label);
#endif
}

static inline void
fbo_attach_texture(fbo_t *fb, int idx_, int tex_)
{
	assert(!fb->is_bound);
	if(fb->free_mask & (1 << idx_)) {
		glDeleteTextures(1, &fb->tex_color[idx_]);
	}
	fb->free_mask &= ~(1 << idx_);
	fb->tex_color[idx_] = tex_;

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx_,
			GL_TEXTURE_2D, tex_, 0);

	GLenum active_attachments[FBO_MAX_COLOR_ATTACHMENTS] = {0};
	for(int i = 0; i < FBO_MAX_COLOR_ATTACHMENTS; i++) {
		active_attachments[i] = fb->tex_color[i] ? GL_COLOR_ATTACHMENT0 + i : GL_NONE;
	}
	fbo_bind(fb);
	glDrawBuffers(FBO_MAX_COLOR_ATTACHMENTS, active_attachments);
	fbo_unbind(fb);
}

static inline void
fb_attach_depth_tex(fbo_t *fb, GLenum target, int tex_)
{
	assert(!fb->is_bound);
	if(fb->free_mask & (1 << (FBO_MAX_COLOR_ATTACHMENTS + 1))) {
		glDeleteTextures(1, &fb->tex_depth);
	}
	fb->free_mask &= ~(1 << (FBO_MAX_COLOR_ATTACHMENTS + 1));
	fb->tex_depth = tex_;

	fbo_bind(fb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, fb->tex_depth, 0);
	fbo_unbind(fb);
}

static inline void
fbo_attach_rb(fbo_t *fb, GLenum type_, int rb_)
{
	assert(!fb->is_bound);
	if(fb->free_mask & (1 << FBO_MAX_COLOR_ATTACHMENTS)) {
		glDeleteRenderbuffers(1, &fb->rb);
	}
	fb->free_mask &= ~(1 << FBO_MAX_COLOR_ATTACHMENTS);

	fbo_bind(fb);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, type_, GL_RENDERBUFFER, rb_);
	fbo_unbind(fb);
}
