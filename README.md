Overview
========

Multiple networks inference on ML Suite v1.5.

![Screencast](doc/img/demo.gif)

ML server (C++)
===============

```bash
# Install required packages
sudo apt install automake cmake libtool

# Build server
./build_server.sh

# Start server
./bin/xdnn_server
```

YOLOv2 client (C++)
===================

```bash
# Install required packages
sudo apt install ffmpeg libopencv-dev libglew-dev libglfw3-dev libopenblas-dev

# Build client
./build_client.sh

# Start video app
./yolov2_video.sh Pedestrians.mp4

# Start webcam app
./yolov2_webcam.sh 0
```

Face detection client (Python3)
===============================

```bash
# Install required packages
pip install python-opencv numpy msgpack msgpack-numpy PyOpenGL glfw

# Start video app
python python/face/mp_video.py
```

Pose estimation client (Python3)
===============================

```bash
# Install required packages
pip install python-opencv numpy msgpack msgpack-numpy PyOpenGL glfw

# Start video app
python python/pose/run_video.py --video Pedestrians.mp4
```
