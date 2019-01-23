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

# TODO: use ./model to extract 3do
# TODO: need more callbacks as for coma draw call to dump obj + offsets
# TODO: check .ai files for more extraordinary draw calls!
