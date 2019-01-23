#!/bin/sh

rm test
gcc bin.c test.c -o test -Wall -pedantic
./test
gcc bin.c restable.c grp.c gob.c pak.c viv.c vpp.c pack.c pack2.c wad23.c res.c dpk4.c dat.c dat2.c rff.c meta.c c_zlib.c c_pack2.c c_lzss.c c_dat.c comdec.c main.c -D_FILE_OFFSET_BITS=64 -I/usr/include/ -L/usr/lib/ -lz -Wall -pedantic -o pup -g
