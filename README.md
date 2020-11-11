Overview
========

Vitis AI 1.2 YOLOv3 demo app.

![Screencast](doc/img/demo.gif)

Prepare model
=============

```bash
Login to Vitis-AI container

Download YOLOv3 model
$ wget https://www.xilinx.com/bin/public/openDownload?filename=dk_yolov3_voc_416_416_65.42G_1.2.zip -O dk_yolov3_voc_416_416_65.42G_1.2.zip

Unzip
$ unzip dk_yolov3_voc_416_416_65.42G_1.2.zip
$ cd dk_yolov3_voc_416_416_65.42G_1.2/quantized/Cloud

Compile
$ conda activate vitis-ai-caffe
$ vai_c_caffe -p deploy.prototxt -c deploy.caffemodel -a /opt/vitis_ai/conda/envs/vitis-ai-caffe/lib/python3.6/site-packages/vaic/arch/DPUCAHX8H/U50/arch.json -o .

Copy xmodel
$ cp deploy.xmodel /path_to/vai-yolov3-demo/model/yolov3.xmodel
```

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
$ sudo apt install ffmpeg libopencv-dev libglew-dev libglfw3-dev libopenblas-dev libunwind-dev cmake

Build client
$ ./build_client.sh

Start video app
$ ./yolov3_video.sh /path_to/video.mp4 --normal

Start webcam app
$ ./yolov3_webcam.sh 0

Start images app
$ ./yolov3_images.sh /path_to/image_list.txt --display-fps 1 --normal
```

