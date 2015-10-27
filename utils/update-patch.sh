#!/bin/sh
path=$1
if [ -e .svn ];then
    svn st $path | grep '^M\|^A' | grep ' openwrt\| feeds' | 
    while read f name
    do
        [ -d "$name" ] && continue
        svn diff $name |sed -e '/^--- \|^+++ /s/\s*(.*)$//g' -e '/^--- \|^+++ /s/ *//g' > patches/${name//\//_}.patch
    done
elif [ -e .git ];then
    git status $path | grep '^#\s*modified:\|^A' | grep ' openwrt\| feeds' | 
    while read f f name
    do
        [ -d "$name" ] && continue
        git diff --no-prefix $name |sed -e '/^--- \|^+++ /s/(.*)//g' -e '/^--- \|^+++ /s/ *//g' > patches/${name//\//_}.patch
    done

else
    echo ...
fi
