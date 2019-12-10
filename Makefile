CC=gcc
CFLAGS=-Wall -std=c11
CFLAGS+=-Iinclude -I.
CFLAGS+=-g -O3 -march=native
LDFLAGS=-lm

LDFLAGS+=$(shell pkg-config --libs libpng)
CFLAGS+=$(shell pkg-config --cflags libpng)

SDL_L+=$(shell pkg-config --libs sdl2) $(shell pkg-config --libs glew)
SDL_C+=$(shell pkg-config --cflags sdl2) $(shell pkg-config --cflags glew)
SDL_L+=$(shell pkg-config --libs SDL2_mixer)
SDL_C+=$(shell pkg-config --cflags SDL2_mixer)

all: pcx model anim sx png2bc3

sanitize: CFLAGS+=-fno-omit-frame-pointer -fsanitize=address
sanitize: all

debug: CFLAGS+=-ggdb3 -O0
debug: all

# TODO: separate into useful subdirs?
HEADERS=include/assets.h\
        include/bc3io.h\
        include/camera.h\
        include/comanche.h\
        include/c3mission.h\
        include/c3pos.h\
        include/c3jim.h\
        include/c3model.h\
        include/comanche.h\
        include/decompress.h\
        include/file.h\
        include/hud.h\
        include/pcxread.h\
        include/pngio.h\
        include/sound.h\
        include/music.h\
        include/sx.h\
        include/physics/rigidbody.h\
        include/physics/aerofoil.h\
        include/physics/heli.h\
        include/plot/helo.h\
        include/world.h\
        include/triggers.h


SX_FILES=src/sx.c\
         src/assets.c\
         src/sound.c\
         src/music.c\
         src/triggers.c\
         src/c3mission.c\
         src/c3object.c\
         src/hud.c\
         src/physics/heli.c\
         src/world.c

# opengl vid module:
HEADERS+=include/vid/gl/vid.h src/vid/gl/terrain.h
SX_FILES+= src/vid/gl/vid.c

pcx: tools/pcx.c Makefile include/decompress.h include/file.h include/pngio.h include/bc3io.h include/pcxread.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

png2bc3: tools/png2bc3.c Makefile include/file.h include/pngio.h include/bc3io.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

model: tools/model.c include/file.h include/c3model.h Makefile
	$(CC) $(CFLAGS) $< -o $@

anim: tools/anim.c include/file.h include/c3model.h Makefile
	$(CC) $(CFLAGS) $< -o $@

sx: src/main.c $(SX_FILES) $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(SDL_C) $< $(SX_FILES) -o $@ $(LDFLAGS) $(SDL_L)

clean:
	rm -f pcx model anim sx
