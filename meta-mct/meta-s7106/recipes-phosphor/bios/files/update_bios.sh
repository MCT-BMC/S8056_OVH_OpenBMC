#!/bin/sh

#echo update: "$@"

doflash=y
biosPath=/tmp/bios/
tarball=bios.tgz
tarPath=$biosPath$tarball

if [ -f $tarPath ]
then 
	echo "bios image found" |systemd-cat
	tar xvzf $tarPath -C $biosPath
else 
	echo "no bios image found"|systemd-cat
	exit 1
fi

for file in ${biosPath}/*
do 
	if [ $(basename $file) != $tarball ]; then
		echo $file |systemd-cat
		biosImage=$file
		break
	fi
	exit 1
done

if test -n "$doflash"; then
    echo "Updating Bios"
    flashcp -v $biosImage /dev/mtd6
    rm -rf biosPath
fi

exit 0
