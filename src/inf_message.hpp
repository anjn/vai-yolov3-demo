#pragma once
#include <vector>
#include <string>

#include "msgpack.hpp"

#include "yolo/yolov3_message.hpp"

namespace demo {

struct inf_request
{
  int image_w;
  int image_h;
  std::vector<uint8_t> image;
  std::vector<std::string> inferences;

  MSGPACK_DEFINE(image_w, image_h, image, inferences);
};

struct inf_reply
{
  yolo::yolov3_reply yolov3;

  MSGPACK_DEFINE(yolov3);
};

}

