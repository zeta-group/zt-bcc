#!/bin/sh

cd "${MESON_BUILD_ROOT}"
[ -d "test" ] || mkdir test && cd test || ( echo "Error running test script: can't have folder 'test' in Meson build root!"; exit 1 )

TEST="$1"
INC="$2"

bcc -x bcs -acc-stats -i "$INC" "${TEST}" "${TEST}.o"
