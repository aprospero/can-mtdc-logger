#!/bin/bash

cd "${0%/*}"

fname="../src/version.h"

git_desc=$(git describe --abbrev=8 --dirty --always --tags)

if [ "X$1" != "X" ] ; then
  git_desc="${git_desc}-$1"
fi

if [ -f $fname ] ; then
  fwords=($(sed '4q;d' ${fname}))  # array of words in line 4 of file 'fname'
  fdesc=${fwords[2]}               # 3rd word of 4th line
  if [ "$fdesc" = "\"$git_desc\"" ] ; then
    echo "git description already up to date."
    exit 0
  fi
fi

cat > ${fname} << EOL
#ifndef _VERSION_H
#define _VERSION_H

#define APP_VERSION "${git_desc}"

#endif  // _VERSION_H
EOL

