#!/bin/sh
# Make sure Remind compiles with all supported languages; show
# tstlang.rem output for each language.

ALL=`grep ^#define lang.h | grep -v '#define LANG' | awk '{print $2}'`

for i in $ALL ; do
    make clean all LANGDEF=-DLANG=$i || exit 1
    ./remind -q -r ../tests/tstlang.rem
done
exit 0
