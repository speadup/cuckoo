#!/bin/sh
vb=99
jb=`cat /proc/cpuinfo | grep processor | sed '/$/G' | wc -l`
dir=openwrt
goal=menuconfig
while [ -n "$1" ]
do
    case $1 in
        -f) 
        shift
        [ -n "$1" ] && cf=$1
        ;;
        -j) 
        shift
        [ -n "$1" ] && jb=$1
        ;;
        -v)
        shift
        [ -n "$1" ] && vb=$1
        ;;
        *config)
        goal=$1
        shift
        ;;
        *)  opt="$opt $1";;
    esac
    shift
done

[ -f "$cf" ] || exit 1
rm -f $dir/.config
cp -f $cf $dir/.config
make -C $dir V=$vb $goal -j $jb
[ -f $dir/.config ] || exit 2
cp -f $dir/.config $cf

