#!/bin/sh

cd "${MESON_BUILD_ROOT}"
[ -d "lib" ] || mkdir lib; cd lib

LIB="$1"
shift

"$@" "${MESON_SOURCE_ROOT}/lib/${LIB}" "./${LIB}.o"
