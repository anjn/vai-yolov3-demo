#!/usr/bin/env bash
set -ex

CFLAGS="-std=c++14 -Iarg -Isrc"
LIBS="-lrt -pthread"

if [[ ${MLSUITE_ROOT} = "" ]] ; then
  echo error
  exit
fi

# ML Suite
make -C ${MLSUITE_ROOT}/apps/yolo/nms

CFLAGS="$CFLAGS -I$MLSUITE_ROOT/xfdnn/rt/xdnn_cpp"
LIBS="$LIBS $MLSUITE_ROOT/xfdnn/rt/libs/libxfdnn.so.v3"
LIBS="$LIBS $MLSUITE_ROOT/ext/boost/libs/libboost_system.so.1.67.0"
LIBS="$LIBS $MLSUITE_ROOT/ext/hdf5/lib/libhdf5.so.103.0.0"
LIBS="$LIBS $MLSUITE_ROOT/ext/hdf5/lib/libhdf5_cpp.so.103.0.0"
LIBS="$LIBS $MLSUITE_ROOT/apps/yolo/nms/nms.o"
LIBS="$LIBS $MLSUITE_ROOT/apps/yolo/nms/nms_20180209/build/libnms.a"

# OpenCV
CFLAGS="$CFLAGS $(pkg-config --cflags opencv)"
LIBS="$LIBS $(pkg-config --libs opencv)"

# OpenGL
#CFLAGS="$CFLAGS $(pkg-config --cflags gl glew glfw3)"
#LIBS="$LIBS $(pkg-config --libs gl glew glfw3)"

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

# yaml-cpp
YAMLCPP=external/yaml-cpp-0.6.2
CFLAGS="$CFLAGS -I$YAMLCPP/include"
LIBS="$LIBS $YAMLCPP/build/libyaml-cpp.a"
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
LIBS="$LIBS $TERMBOX/build/src/libtermbox.a"
if [ ! -e $TERMBOX/build/src/libtermbox.a ] ; then
  pushd $TERMBOX
  ./waf configure
  ./waf
  popd
fi

## # XRT
## XRT=external/XRT-2018.2_XDF
## CFLAGS="$CFLAGS -I$XRT/src/runtime_src/"
## #CFLAGS="$CFLAGS -I$XRT/src/runtime_src/driver/include"
## CFLAGS="$CFLAGS -I$XRT/src/runtime_src/driver/xclng/xrt/user_gem"
## LIBS="$LIBS $XRT/src/runtime_src/driver/xclng/xrt/user_gem/libxclgemdrv.a"
## if [ ! -e $XRT/src/runtime_src/driver/xclng/xrt/user_gem/libxclgemdrv.a ] ; then
##   make -C $XRT/src/runtime_src/driver/xclng/xrt/user_gem
## fi

# cppzmq
CFLAGS="$CFLAGS -Iexternal/cppzmq-4.4.1"

# arg
CFLAGS="$CFLAGS -Iexternal/arg-master"

#CFLAGS="$CFLAGS -O3"
CFLAGS="$CFLAGS -g"

programs="xdnn_server"

if [ $# -ge 1 ] ; then
  programs=$*
fi

for e in $programs; do
  mkdir -p bin
  g++ -o bin/$e src/$e.cpp $CFLAGS $LIBS
done

