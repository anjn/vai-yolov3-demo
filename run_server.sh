#!/usr/bin/env bash

pushd /workspace/alveo-hbm
source overlay_settle.sh
popd

./bin/inf_server
