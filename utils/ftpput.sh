#!/bin/sh
host=192.168.19.192
host=192.168.18.24
port=21
user=upload
pass=1sjwt6uF11e1Sdwz
path=/mirror/download

[ -n "$1" ] || exit 1
name=`basename $1`
dir=`dirname $1`
[ -n "$name" ] || exit 1
[ -e "$dir/$name" ] || exit 1
clear
(cd $dir;md5sum -b $name >/tmp/$name.md5)
cat /tmp/$name.md5
ftp -vnp <<EOF
open $host $port
user $user $pass
binary
put $dir/$name     $path/$name
put /tmp/$name.md5 $path/$name.md5
bye
EOF
rm -f /tmp/$name.md5

