#!/bin/sh

rm -f *.nds *.elf *.gba *.arm9

version=`cat version`

make clean
make ADDITIONAL_CFLAGS="-DDS81_DISABLE_FAT -DDS81_VERSION=\"\\\"$version\\\"\""

mv ds81.nds ds81-nofat.nds
mv ds81.ds.gba ds81-nofat.ds.gba

make clean
make ADDITIONAL_CFLAGS="-DDS81_VERSION=\"\\\"$version\\\"\""

./fat_patch.sh
