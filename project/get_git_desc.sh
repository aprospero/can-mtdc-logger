#!/bin/bash

cd "${0%/*}"

git_desc=$(git describe --abbrev=8 --dirty --always --tags)

if [ "X$1" = "X" ] ; then
  suffix=""
else
  suffix="-$1"
fi

cat > ../code/version.h << EOL
#ifndef _VERSION_H
#define _VERSION_H

#define APP_VERSION "${git_desc}${suffix}"

#endif  // _VERSION_H
EOL

