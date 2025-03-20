#!/bin/sh

if [ $# -ne 1 ] ; then
	echo usage: $1 \<directory to fix for sharing in the container\>
	exit 1
fi
find $1 -type f -exec chmod 664 {} \;
find $1 -type d -exec chmod 775 {} \;
