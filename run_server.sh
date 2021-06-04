#!/usr/bin/env bash

source /opt/xilinx/xrt/setup.sh

export INTERNAL_BUILD=1

pushd $(dirname $(readlink -f $0))
#valgrind --leak-check=full ./bin/inf_server
./bin/inf_server
