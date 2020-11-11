#!/usr/bin/env bash

#set -ex

trap 'stty sane' SIGINT

source /opt/xilinx/xrt/setup.sh > /dev/null

image_list=$1; shift

frame_w=416
frame_h=416

./bin/opencv_input_images $image_list | ./bin/yolov3_client - $frame_w $frame_h $*

stty sane

