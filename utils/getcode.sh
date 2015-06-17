#!/bin/sh
url=http://192.168.18.24/svn/openwrt/trunk
cfg=feeds.conf
plus=plus
temp=temp
repo=repo
export LANG=en_US.UTF-8

while [ -n "$1" ]
do
    case $1 in
        -f) 
        shift
        [ -n "$1" ] && cfg=$1
        ;;
        -t)
        shift
        [ -n "$1" ] && temp=$1
        ;;
        *)
        ;;
    esac
    shift
done


[ ! -f ${cfg} ] && exit 1
name=openwrt
ver=`svn info ${url} | sed -e '/Revision:/!d' -e 's/Revision: //'`
[ -n "${ver}" ] || exit 1
mkdir -p ${temp}/${repo}
rm -rf ${temp}/${repo}/${name}*
svn co -q ${url} -r ${ver} ${temp}/${repo}/${name} || exit 1
svn export -q ${temp}/${repo}/${name} -r ${ver} ${temp}/${name}-${ver} || exit 1
(cd ${temp}; tar zcf ${name}-${ver}.tar.gz ${name}-${ver})
rm -rf ${temp}/${name}
mv ${temp}/${name}-$ver ${temp}/$name
find ${temp}/${name}/package -name 'Makefile' | sed -e '/\/files\/\|\/src\//d' -e 's/\/Makefile//' -e 's/.*\///' >${temp}/list.txt
echo "%openwrt@trunk:${ver}" > ${temp}/version.txt
cat ${cfg} | grep -v '^#' |
while read type name url buff
do
    rm -rf ${temp}/${repo}/${name}*
    git clone ${url} ${temp}/${repo}/${name} || exit 1
    ver=`cd ${temp}/${repo}/${name}; git show | sed -e '1!d' -e 's/commit //'`
    (cd ${temp}/${repo}/${name}; git archive --format tar --prefix ${name}-${ver}/ -o ../../${name}-${ver}.tar HEAD)
    rm -f ${temp}/${name}-${ver}.tar.gz
    gzip -9 ${temp}/${name}-${ver}.tar
    (cd ${temp}; tar zxf ${name}-${ver}.tar.gz)
    [ -d ${temp}/${plus} ] || mkdir -p ${temp}/${plus}
    rm -rf ${temp}/${plus}/${name}
    mv ${temp}/${name}-${ver} ${temp}/${plus}/${name}
    echo "%${name}@${ver}" >> ${temp}/version.txt
    [ ${name} = "luci" ] && continue
    find ${temp}/${plus}/${name} -name 'Makefile' | sed -e '/\/files\/\|\/src\//d' -e 's/\/Makefile$//' | sort >${temp}/path.txt
    sed -e 's/.*\///' ${temp}/path.txt >>${temp}/list.txt
    sort ${temp}/list.txt > ${temp}/list.txt.tmp
    sort -u ${temp}/list.txt.tmp >${temp}/list.txt
    diff -b -B ${temp}/list.txt ${temp}/list.txt.tmp | sed -e '/^> /!d' -e 's@^> @@' |
    while read item
    do
        [ -n "${item}" ] || continue
        sed "/\/${item}\$/!d" ${temp}/path.txt | 
        while read item1
        do
            echo ${item1}
            rm -rf ${item1} 
        done
    done
done

#mv $temp/*.tar.gz .
vim -d version.txt $temp/version.txt
