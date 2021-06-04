Overview
========

Vitis AI 1.3 YOLOv3 demo app.

![Screencast](doc/img/demo.gif)

ML server (C++)
===============

```bash
Login to Vitis-AI container

Install required packages
$ sudo apt install automake cmake libtool

Build server
$ ./build_server.sh

Start server
$ ./run_server.sh
```

YOLOv3 client (C++)
===================

```bash
Install required packages
$ sudo apt install ffmpeg libopencv-dev libglew-dev libglfw3-dev libopenblas-dev libunwind-dev cmake libxml2-dev

Build client
$ ./build_client.sh

Start video app
$ ./yolov3_video.sh /path_to/video.mp4 --normal

Start webcam app
$ ./yolov3_webcam.sh 0

Start images app
$ ./yolov3_images.sh /path_to/image_list.txt --display-fps 1 --normal
```

