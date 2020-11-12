#!/usr/bin/env bash

trap 'stty sane' SIGINT

dir=$(dirname $(readlink -f $0))
cd $dir

source /opt/xilinx/xrt/setup.sh > /dev/null

video=$1; shift

eval $(./bin/opencv_size "$video")

frame_w=$((416*1))
frame_h=$((frame_w*height/width))

ffmpeg -stream_loop -1 -i "$video" -filter_complex scale=${frame_w}:${frame_h} -pix_fmt rgb32 -f rawvideo - 2> /dev/null | ./bin/yolov3_client - $frame_w $frame_h $*

stty sane
