#!/usr/bin/env bash

set -ex

trap 'stty sane' SIGINT

video=$1; shift

eval $(./bin/opencv_size $video)

frame_w=416
frame_h=$((frame_w*height/width))

#ffmpeg -stream_loop -1 -i $video -filter_complex scale=${frame_w}:${frame_h} -pix_fmt rgb32 -f rawvideo - < /dev/null | ./bin/yolov2 - $frame_w $frame_h $*

# For older ffmpeg
#ffmpeg -i $video -filter_complex scale=${frame_w}:${frame_h} -pix_fmt rgb32 -f rawvideo - < /dev/null | ./bin/yolov3_client - $frame_w $frame_h $*
ffmpeg -i $video -filter_complex scale=${frame_w}:${frame_h} -pix_fmt rgb32 -f rawvideo - 2> /dev/null | ./bin/yolov3_client - $frame_w $frame_h $*

