#!/usr/bin/env bash

source /opt/xilinx/xrt/setup.sh

export INTERNAL_BUILD=1

pushd /workspace/alveo-hbm
source overlay_settle.sh
popd

pushd $(dirname $(readlink -f $0))
./bin/inf_server
