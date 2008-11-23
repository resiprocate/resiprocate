#! /bin/sh

# lndir - create shadow link tree
#
# $XConsortium: lndir.sh,v 1.8 91/04/15 17:55:03 rws Exp $
#
# Used to create a copy of the a directory tree that has links for all
# non- directories (except those named CVS, RCS, or SCCS).  If you are
# building the distribution on more than one machine, you should use
# this script.
#
# If your master sources are located in /usr/local/src/X and you would like
# your link tree to be in /usr/local/src/new-X, do the following:
#
# 	%  mkdir /usr/local/src/new-X
#	%  cd /usr/local/src/new-X
# 	%  lndir ../X

USAGE="Usage: $0 fromdir [todir]"

if [ $# -lt 1 -o $# -gt 2 ]
then
	echo "$USAGE"
	exit 1
fi

DIRFROM=$1

if [ $# -eq 2 ];
then
	DIRTO=$2
else
	DIRTO=.
fi

if [ ! -d $DIRTO ]
then
        mkdir -p $DIRTO
fi

cd $DIRTO

if [ ! -d $DIRFROM ]
then
	echo "$0: $DIRFROM is not a directory"
	echo "$USAGE"
	exit 2
fi

pwd=`pwd`

if [ `(cd $DIRFROM; pwd)` = $pwd ]
then
	echo "$pwd: FROM and TO are identical!"
	exit 1
fi

for file in `ls -af $DIRFROM`
do
    if [ ! -d $DIRFROM/$file ]
    then
#	    echo "linking $DIRFROM/$file to pwd"
	    ln -s $DIRFROM/$file . > /dev/null 2>&1 
    else
	    if [ $file != CVS -a $file != RCS -a $file != SCCS -a $file != . -a $file != .. ]
	    then
		    echo $file:
		    mkdir -p $file > /dev/null 2>&1 
		    (cd $file
		     pwd=`pwd`
		     case "$DIRFROM" in
			     /*) ;;
			     *)  DIRFROM=../$DIRFROM ;;
		     esac
		     if [ `(cd $DIRFROM/$file; pwd)` = $pwd ]
		     then
			    echo "$pwd: FROM and TO are identical!"
			    exit 1
		     fi
		     $0 $DIRFROM/$file
		    )
	    fi
    fi
done

exit 0
