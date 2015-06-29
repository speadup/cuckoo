#!/bin/sh
url=svn://svn.openwrt.org/openwrt
branch=branches/chaos_calmer
cfg=feeds.conf
feeds=feeds
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
        -m)
        shift
        [ -n "$1" ] && mirror=$1
        ;;
        *)
        ;;
    esac
    shift
done

mirror=${mirror%%/}

[ ! -f ${cfg} ] && exit 1
name=openwrt
[ -n "${mirror}" ] && url=${mirror}/svn/${name}
ver=`svn info ${url}/${branch} | sed -e '/Last Changed Rev:/!d' -e 's/.*: //'`
[ -n "${ver}" ] || exit 1
mkdir -p ${temp}/${repo}
rm -rf ${temp}/${repo}/${name}*
svn co -q ${url}/${branch} -r ${ver} ${temp}/${repo}/${name} || exit 1
svn export -q ${temp}/${repo}/${name} -r ${ver} ${temp}/${name}-${ver} || exit 1
(cd ${temp}; tar zcf ${name}-${ver}.tar.gz ${name}-${ver})
rm -rf ${temp}/${name}
mv ${temp}/${name}-$ver ${temp}/$name
find ${temp}/${name}/package -name 'Makefile' | sed -e '/\/files\/\|\/src\//d' -e 's/\/Makefile//' -e 's/.*\///' >${temp}/list.txt
echo "%openwrt@${branch}:${ver}" > ${temp}/version.txt
find  ${temp}/${name} -type d -empty -delete
cat ${cfg} | grep -v '^#' |
while read type name url buff
do
    [ -n "${mirror}" ] && url=${url//*:\/\//${mirror}/git/}
    commit=`echo "${url}" | awk -F\# '{print $2}'`
    url=${url%#*}
    branch=`echo "${url}" | awk -F\; '{print $2}'`
    url=${url%;*}
    rm -rf ${temp}/${repo}/${name}*
    [ -n "${branch}" ] && (git clone --depth 1 --branch ${branch} ${url} ${temp}/${repo}/${name} || exit 1)
    [ -z "${branch}" ] && (git clone --depth 1 ${url} ${temp}/${repo}/${name} || exit 1)
    ver=`cd ${temp}/${repo}/${name}; git show | sed -e '1!d' -e 's/commit //'`
    (cd ${temp}/${repo}/${name}; git archive --format tar --prefix ${name}-${ver}/ -o ../../${name}-${ver}.tar HEAD)
    rm -f ${temp}/${name}-${ver}.tar.gz
    gzip -9 ${temp}/${name}-${ver}.tar
    (cd ${temp}; tar zxf ${name}-${ver}.tar.gz)
    [ -d ${temp}/${feeds} ] || mkdir -p ${temp}/${feeds}
    rm -rf ${temp}/${feeds}/${name}
    mv ${temp}/${name}-${ver} ${temp}/${feeds}/${name}
    echo "%${name}@${branch:-master}:${ver}" >> ${temp}/version.txt
    [ ${name} = "luci" ] && continue
    find ${temp}/${feeds}/${name} -name 'Makefile' | sed -e '/\/files\/\|\/src\//d' -e 's/\/Makefile$//' | sort >${temp}/path.txt
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
    find  ${temp}/${feeds}/${name} -type d -empty -delete
done

#mv $temp/*.tar.gz .
vim -d version.txt $temp/version.txt
