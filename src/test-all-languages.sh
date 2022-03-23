#!/bin/sh
# Make sure Remind compiles with all supported languages; show
# tstlang.rem output for each language.

ALL=`grep ^#define lang.h | grep -v '#define LANG' | awk '{print $2}'`

OUTPUT_COMPILED=lang-compiled.out
OUTPUT_RUNTIME=lang-runtime.out
cat /dev/null > $OUTPUT_COMPILED
cat /dev/null > $OUTPUT_RUNTIME
for i in $ALL ; do
    make clean
    make -j`nproc` all LANGDEF=-DLANG=$i || exit 1
    ./remind -q -r ../tests/tstlang.rem >> $OUTPUT_COMPILED 2>&1
done

# Rebuild English version
make clean
make -j`nproc` all || exit 1

ALL=`ls ../include/lang/*.rem`
for i in $ALL; do
    ./remind -q -r "-ii=\"$i\"" ../tests/tstlang.rem >> $OUTPUT_RUNTIME 2>&1
done

exit 0
