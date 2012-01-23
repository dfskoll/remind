#!/bin/sh

ALL=`grep ^#define lang.h | grep -v '#define LANG' | awk '{print $2}'`
echo $ALL

for i in $ALL ; do
    make clean all LANGDEF=-DLANG=$i || exit 1
    ./remind ../tests/tstlang.rem
done
exit 0
