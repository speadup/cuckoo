#!/bin/sh
basedir=`dirname $0`
basedir=`cd $basedir/..;pwd`
echo $basedir

[ -f feeds.conf ] || cp $basedir/feeds.conf .
[ -d utils/ ] || ln -s $basedir/utils . 
[ -d download/ ] || ln -s $basedir/download .
[ -d defconfig/ ] || (mkdir -p defconfig; cp $basedir/defconfig/config-* defconfig)
[ -d patches/ ] || (mkdir -p patches; cp $basedir/patches/*.patch patches)

