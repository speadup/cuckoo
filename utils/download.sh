#!/bin/sh
pwd=$PWD
cd `dirname $0`
export PATH=$PWD:$PATH
cd $pwd
mkdir -p tmp
find tmp -maxdepth 1 -name 'debug-*.txt' -delete
make V=99 download | tee build.log

