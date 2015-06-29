#!/bin/sh
path=$1
svn st $path | grep '^M\|^A' | grep ' openwrt\| feeds' | 
while read f name
do
    [ -d "$name" ] && continue
    svn diff $name |sed -e '/^--- \|^+++ /s/(.*)//g' -e '/^--- \|^+++ /s/ *//g' > patches/${name//\//_}.patch
done
