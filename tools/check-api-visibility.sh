#!/bin/bash

# Check that private headers aren't included in public ones.
if grep "include.*private.h" $(find src -name '*\.h' -not -regex '.*private.h');
then
  echo "Private headers shouldn't be included in public ones."
  exit 1
fi

# Check that libmanette.h contains all the public headers.
for header in $(find src -name '*.h' -not -regex '.*private.h' -not -regex '.*/libmanette.h');
do
  if ! grep -q "$(basename $header)" src/libmanette.h;
  then
    echo "The public header" $(basename $header) "should be included in libmanette.h."
    exit 1
  fi
done
