#!/bin/sh
src=${1-temp}
dest=${2-.}


add()
{
    if [ -d .svn ];then
        svn add $1
    elif [ -d .git ];then
        git add $1
    else
        echo "error ... no .git or .svn $1"
        exit 0
    fi
}
del()
{
    if [ -d .svn ];then
        svn del $1
    elif [ -d .git ];then
        git rm $1
    else
        echo "error ... no .git or .svn $1"
        exit 0
    fi
}
update_delete()
{
    local subdir curdir=$1
    while read subdir
    do
        [ "$subdir" = "." -o "$subdir" = ".." -o "$subdir" = ".svn" ] && continue
        [ -L $dest/$curdir/$subdir ] && continue
        [ ! -e $src/$curdir/$subdir ] && (echo $dest/$curdir/$subdir;svn del $dest/$curdir/$subdir) && continue
        [ -d $dest/$curdir/$subdir ] && update_delete $curdir/$subdir
    done <<EOF
`ls -a $dest/$curdir`
EOF
}

update_copy()
{
    local subdir curdir=$1
    while read subdir
    do
        [ "$subdir" = "." -o "$subdir" = ".." -o "$subdir" = ".svn" ] && continue
        [ -L $src/$curdir/$subdir ] && continue
        [ -f $dest/$curdir/$subdir ] && cp -a $src/$curdir/$subdir $dest/$curdir/$subdir && continue
        [ ! -e $dest/$curdir/$subdir ] && (
            cp -a $src/$curdir/$subdir $dest/$curdir/$subdir;
            svn add $dest/$curdir/$subdir) && continue
        [ -d $dest/$curdir/$subdir ] && update_copy $curdir/$subdir
    done <<EOF
`ls -a $src/$curdir`
EOF
}

update_delete openwrt
update_delete feeds/luci
update_delete feeds/packages
update_delete feeds/routing
update_delete feeds/oldpackages
cp -a $src/version.txt $dest/
update_copy openwrt
update_copy feeds/luci
update_copy feeds/packages
update_copy feeds/routing
update_copy feeds/oldpackages
