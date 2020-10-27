#!/usr/bin/env bash
set -ex

CFLAGS="-std=c++14 -Iarg -Isrc"
LIBS="-lrt -pthread"
LIBS="$LIBS -lxml2"

# OpenCV
CFLAGS="$CFLAGS $(pkg-config --cflags opencv)"
LIBS="$LIBS $(pkg-config --libs opencv)"

# OpenGL
CFLAGS="$CFLAGS $(pkg-config --cflags gl glew glfw3)"
LIBS="$LIBS $(pkg-config --libs gl glew glfw3)"

# zeromq
ZMQ=external/zeromq-4.3.2
CFLAGS="$CFLAGS -I$ZMQ/include"
LIBS="$LIBS $ZMQ/src/.libs/libzmq.a"
if [ ! -e $ZMQ/src/.libs/libzmq.a ] ; then
  pushd $ZMQ
  ./autogen.sh
  ./configure --enable-static
  make -j8
  popd
fi

# msgpack-c
MSGPACK=external/msgpack-3.2.0
CFLAGS="$CFLAGS -I$MSGPACK/include"
LIBS="$LIBS $MSGPACK/build/libmsgpackc.a"
if [ ! -e $MSGPACK/build/libmsgpackc.a ] ; then
  pushd $MSGPACK
  mkdir -p build
  cd build
  cmake ..
  make -j8
  popd
fi

# cppzmq, arg
CFLAGS="$CFLAGS -Iexternal/cppzmq-4.4.1"
CFLAGS="$CFLAGS -Iexternal/arg-master"

#CFLAGS="$CFLAGS -O3"
CFLAGS="$CFLAGS -g"

programs="yolov2_client opencv_size opencv_input"

if [ $# -ge 1 ] ; then
  programs=$*
fi

for e in $programs; do
  mkdir -p bin
  g++ -o bin/$e src/$e.cpp $CFLAGS $LIBS
done
