#!/bin/sh
find patches/ -name '*.patch' | sort |
while read item
do
    patch -p0 $@ -i $item
done

