#!/usr/bin/env bash

set -ex

video=${1:-0}; shift

eval $(./bin/opencv_size $video)

frame_w=416
frame_h=$((frame_w*height/width))

./bin/opencv_input $video | ./bin/yolov3_client - $frame_w $frame_h $*
