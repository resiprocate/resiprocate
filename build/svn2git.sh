#!/bin/bash

set -e
set -x

#--- Main configuration for this script:   ------------------------

SVN_BASE=https://svn.resiprocate.org/rep/resiprocate/
GIT_SERVER_URL=ssh://git@github.com/resiprocate/resiprocate.git

if [ -z "${CONVERT_BASE}" ];
then
  CONVERT_BASE=/tmp/svn2git
fi

#------------------------------------------------------------------

mkdir -p "${CONVERT_BASE}"
WORK_ROOT=`mktemp -d -p "${CONVERT_BASE}"`

CLONE1=${WORK_ROOT}/clone1
AUTHORS_FILE=${WORK_ROOT}/authors.txt

# This section of code can pull the usernames from the SVN logs (useful if they are email addresses):
#svn log -q $SVN_BASE | \
#   awk -F '|' \
#     '/^r/ {sub("^ ", "", $2); sub(" $", "", $2); print $2" = "$2" "}' | \
#  sort -u > ${AUTHORS_FILE}

#cat ${AUTHORS_FILE} | \
#  sed -e 's/= \(.*\)$/= <test@resiprocate.org>/' \
#   > ${AUTHORS_FILE}-test
#AUTHORS_FILE=${AUTHORS_FILE}-test

# This code takes the authors file that is checked in to SVN
wget -O ${AUTHORS_FILE} ${SVN_BASE}/main/svn-authors.txt

git svn clone \
  --trunk=main --tags=tags --branches=branches \
  --no-metadata \
  -A ${AUTHORS_FILE} \
  $SVN_BASE $CLONE1

CLONE2=${WORK_ROOT}/clone2
mkdir ${CLONE2}
cd ${CLONE2}
git init --bare .
git symbolic-ref HEAD refs/heads/trunk

cd ${CLONE1}
git remote add bare ${CLONE2}
git config remote.bare.push 'refs/remotes/*:refs/heads/*'
git push bare

cd ${CLONE2}
git branch -m trunk master
git for-each-ref --format='%(refname)' refs/heads/tags | cut -d / -f 4 | while read ref; \
do
git tag "$ref" "refs/heads/tags/$ref"
git branch -D "tags/$ref"
done

git remote add origin $GIT_SERVER_URL
git config branch.master.remote origin
git config branch.master.merge refs/heads/master
git push --tags origin master
git push --all

