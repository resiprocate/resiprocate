#! /bin/sh

PROJECT=Netxx
VERSION_FILE=docs/VERSION
VERSION=`head -1 $VERSION_FILE | perl -pe 's/\s+.*$//'`
CVSTAG=`head -1 $VERSION_FILE  | perl -pe 's/^[\d.]+\s+\d+\s+([^\s]+).*$/$1/'`
DESTDIR="$PROJECT-$VERSION"
DOCBOOK_XSL=$HOME/develop/xslt-tools/stylesheets/docbook2html.xsl
README_XSL=$HOME/develop/project-xslt/stylesheets/readme.xsl
W3M="w3m -cols 76 -dump"
XSLT="xsltproc"

echo "===> Making $PROJECT release (Version == $VERSION) (CVSTAG == $CVSTAG)"

if [ ! -r $DOCBOOK_XSL ] ; then
    echo "missing $DOCBOOK_XSL"
    exit 1
fi

if [ ! -r $README_XSL ] ; then
    echo "missing $README_XSL"
    exit 1
fi

# checkout the correct tag/branch from CVS
cvs -q checkout -r $CVSTAG -d $DESTDIR $PROJECT

if [ ! -d $DESTDIR ] ; then
    echo "cvs checkout failed"
    exit 1
fi

# remove the CVS directories
find $DESTDIR -name CVS -type d | xargs rm -rf

# create the README, INSTALL and like files
TMPFILE=tmp.html
(
    cd $DESTDIR

    $XSLT $README_XSL docs/project/project.xml > $TMPFILE
    $W3M $TMPFILE > README

    $XSLT $DOCBOOK_XSL docs/manual/get_started.xml > $TMPFILE
    $W3M $TMPFILE > INSTALL

    $XSLT $DOCBOOK_XSL docs/manual/credits.xml > $TMPFILE
    $W3M $TMPFILE > docs/CREDITS

    $XSLT $DOCBOOK_XSL docs/manual/todo.xml > $TMPFILE
    $W3M $TMPFILE > docs/TODO

    rm $TMPFILE
)

# create a tarball
tar czvf ${DESTDIR}.tar.gz $DESTDIR

# remove the directory that CVS created
rm -rf $DESTDIR
