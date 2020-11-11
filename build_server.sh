#!/usr/bin/env bash
set -ex

CFLAGS="-std=c++17 -Iarg -Isrc"
LIBS="-lrt -pthread -lstdc++ -ldl -lunwind -lglog"

# VART
LIBS="$LIBS -lvart-runner -lxir"

# OpenCV
CFLAGS="$CFLAGS $(pkg-config --cflags opencv)"
LIBS="$LIBS $(pkg-config --libs opencv)"

# zeromq
ZMQ=external/zeromq-4.3.2
CFLAGS="$CFLAGS -I$ZMQ/include"
LIBS="$ZMQ/src/.libs/libzmq.a $LIBS"
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
LIBS="$MSGPACK/build/libmsgpackc.a $LIBS"
if [ ! -e $MSGPACK/build/libmsgpackc.a ] ; then
  pushd $MSGPACK
  mkdir -p build
  cd build
  cmake ..
  make -j8
  popd
fi

# yaml-cpp
YAMLCPP=external/yaml-cpp-0.6.2
CFLAGS="$CFLAGS -I$YAMLCPP/include"
LIBS="$YAMLCPP/build/libyaml-cpp.a $LIBS"
if [ ! -e $YAMLCPP/build/libyaml-cpp.a ] ; then
  pushd $YAMLCPP
  mkdir -p build
  cd build
  cmake ..
  make -j8
  popd
fi

# termbox
TERMBOX=external/termbox-1.1.2
CFLAGS="$CFLAGS -I$TERMBOX/src"
LIBS="$TERMBOX/build/src/libtermbox.a $LIBS"
if [ ! -e $TERMBOX/build/src/libtermbox.a ] ; then
  pushd $TERMBOX
  ./waf configure
  ./waf
  popd
fi

# spdlog
SPDLOG=external/spdlog-1.8.1
CFLAGS="$CFLAGS -I$SPDLOG/include"
LIBS="$SPDLOG/build/libspdlog.a $LIBS"
if [ ! -e $SPDLOG/build/libspdlog.a ] ; then
  pushd $SPDLOG
  mkdir -p build
  cd build
  cmake ..
  make -j8
  popd
fi

# cppzmq
CFLAGS="$CFLAGS -Iexternal/cppzmq-4.4.1"

# arg
CFLAGS="$CFLAGS -Iexternal/arg-master"

CFLAGS="$CFLAGS -O3"
#CFLAGS="$CFLAGS -g -DDEBUG"

programs="inf_server yolov3_test"

if [ $# -ge 1 ] ; then
  programs=$*
fi

for e in $programs; do
  mkdir -p bin
  g++ -o bin/$e src/$e.cpp $CFLAGS $LIBS
done

