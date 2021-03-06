# sioux is not comanche

sioux is a drop-in replacement engine for the 1997 game *comanche 3*, similar
to how gzdoom is an engine for the 1993 game doom (only that gzdoom is complete
and working).

![](doc/img0.jpg)
![](doc/img1.jpg)
![](doc/img2.jpg)
![](doc/img3.jpg)
![](doc/img4.jpg)
![](doc/img5.jpg)
![](doc/img6.jpg)
![](doc/img7.jpg)
![](doc/img8.jpg)
![](doc/img9.jpg)
![](doc/img10.jpg)
![](doc/img11.jpg)
![](doc/img12.jpg)
![](doc/img13.jpg)
![](doc/img14.jpg)

back then i loved the game and was blown away by the voxel engine
and the cohesive mood introduced into the graphics by the use of
palettes.

playing it today i still think it's a great game. but.

* a flight simulator with a camera projection that does not allow to pitch and
  bank without weird distortions?

* faking similar colour mood by multiplying a consistent palette is nice.
  today this job is usually done by global illumination.

# compile

the build system is a simple makefile with some pkg-config and hardcoded paths:

```
$ make
```


# run

you'll need the original game cd, in particular the resource.res file (i know,
the gold edition has a pff instead but my old cdrom didn't yet).

TODO: describe how to extract the res with the tools in ```tools/``` as
well as the external tool ```pup/``` by vladimir stupin.

```
$ ./sx
```

you can also pass ```--fullscreen``` and play with ```--lod 1``` (1 is highest,
4 is default, higher will be lower detail). if you want to start another
mission, try ```./sx c1m3.mis``` or similar. in fact c1m3 is probably the only mission
yet which you can play about as intended and actually win :)

For ```SDL2_mixer``` to work with fluidsynth you need to set ```SDL_SOUNDFONTS```:

```
$ export SDL_SOUNDFONTS=<path to soundfonts directory>/<soundfont filename>
```

# controls

keyboard bindings are

![](doc/keymap.svg)

or, if you have a dual shock 4 controller:

![](doc/gamepad.svg)

and you may want to read [the helicopter flying handbook](https://www.faa.gov/regulations_policies/handbooks_manuals/aviation/helicopter_flying_handbook/).

there's also 'p' for pause, 'f' for opening bay door, 'g' for raise/lower gear.

currently the keymap is hardcoded in ```src/vid/gl/vid.c``` and prepared for an english
dvorak layout.



# known issues

this is just release-early-release-often code for your perusal. it's totally
not playable. in particular:

* i implemented some of the trigger-based scripting system but it's by no means complete

* enemy ai makes them not crash immediately (hey, flying helicopters is hard!) but they're really dumb

* loaded the scene and all objects and their textures, but only very few
are animated

* did not implement weapons (just infinite amounts of dummy rockets)

# TODO items

* shading (currently it's just pretty much displaying the texture colour and normal)

* global illumination, or some close fake

* horizon maps or ray tracing for terrain shadows and radar/stealth

* simulate weather and winds in the terrain (maybe i'll settle for incompressible fluids here)

* more sophisticated flight dynamics (got aerofoil from flight gear, but rotor needs improvement)

* take local winds/turbulence into account when simulating flight dynamics

* port rendering to vulkan so we can use RTX


