#!/bin/sh
src=${1-temp}
dest=${2-.}

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
update_delete plus/luci
update_delete plus/packages
update_delete plus/routing
update_delete plus/oldpackages
cp -a $src/version.txt $dest/
update_copy openwrt
update_copy plus/luci
update_copy plus/packages
update_copy plus/routing
update_copy plus/oldpackages
