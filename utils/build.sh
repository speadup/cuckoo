#!/bin/sh
cf=
cl=
df=
jb=`cat /proc/cpuinfo | grep processor | sed '/$/G' | wc -l`
vb=99
dir=openwrt

if [ `ps --no-headers -oppid -p$$` -gt 1 ]; then
    name=`basename $0`
    ppid=`ps --no-headers -oppid -p$$`
    if [ `ps --no-headers -ocomm -p$ppid` != $name ];then
        $0 $@ 
        sleep 1
        tail --pid=`fuser ${name%%.*}.log 2>&1 | awk -F\  '{print $2}'` -f ${name%%.*}.log
        exit 0
    else
        exec 0</dev/null
        exec 1>${name%%.*}.log
        exec 2>&1
        setsid $0 $@ &
        exit 0 
    fi
fi

while [ -n "$1" ]
do
    case $1 in
        -f) 
        shift
        [ -n "$1" ] && cf=$1
        ;;
        -c) cl=1;;
        -d) dc=1;;
        -j) 
        shift
        [ -n "$1" ] && jb=$1
        ;;
        -v)
        shift
        [ -n "$1" ] && vb=$1
        ;;
        *)  opt="$opt $1";;
    esac
    shift
done

[ -f $dir/Makefile -o -f $dir/makefile ] || dir=.
[ ! -f $dir/Makefile -a ! -f $dir/makefile ] && echo "File [ Makefile ] not exists!" && exit 1

if [ -n "$cl" -o -n "$dc" ];then
    printf "==%18s==========================================\n" "`date +'%Y-%m-%d %H:%M:%S'`"
    rm -rf $dir/logs $dir/bin $dir/release $dir/tmp release
    find . \( -path ./openwrt/build_dir -o \
             -path ./openwrt/staging_dir -o \
             -path ./download \) -prune -depth -o \
             \( -type f \( -name '*.orig' -o -name '*.rej' -o -name '*.o' \) -delete \)
fi
if [ -n "$dc" ];then
    make -C $dir distclean V=$vb
    rm -rf $dir/dl $dir/download
    rm -f $dir/scripts/config/zconf.lex.c
    rm -f $dir/scripts/config/mconf_check
    rm -rf $dir/key-build $dir/key-build.pub
fi

[ -n "$cf" ] && [ ! -f "$cf" ] && echo "File [ $cf ] not exists!" && exit 1
[ -n "$cf" ] && [ -f "$cf" ] && cp $cf $dir/.config
[ ! -f $dir/.config ] && echo "File [ .config ] not exists!" && exit 1
[ -z "$cl" -a -z "$dc" ] && make -j $jb V=$vb package/luci/clean

mkdir -p $dir/tmp
find $dir/tmp -maxdepth 1 -name 'debug-*.txt' -delete
export PATH=$PWD/utils:$PATH
printf "==%18s==========================================\n" "`date +'%Y-%m-%d %H:%M:%S'`"
t1=`date +%s`

./$dir/scripts/getver.sh > ./$dir/version
make -C $dir -j $jb V=$vb $opt

t2=`date +%s`
printf "==%18s==========================================\n" "`date +'%Y-%m-%d %H:%M:%S'`"
t3=`echo "($t2 - $t1) / 60" | bc`
printf "==%18dm==========================================\n" $t3
