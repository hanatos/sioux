#!/bin/bash
make
make -C pup
mkdir -p res
./pup/pup --plugin res --unpack resource.res res/
mkdir -p tmp
> failed
for i in res/*pcx
do
  echo $i
  ./pcx $i tmp/$(basename ${i%.pcx}.png) || echo $i >> failed
done

make png2bc3

# include high-res cockpit into comanche:
convert \( tmp/com08.png -resize 2000x400 -transparent "#000" \) -compose over \
        \( tmp/cc_pit.png -crop 640x240+0+240 \) \
        -gravity north -composite com08.png
./png2bc3 com08.png tmp/com08.bc3

# use ./model to extract 3do to obj if you're interested, and ./anim for
# animation curves.
