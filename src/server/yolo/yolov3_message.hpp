#pragma once

#include <chrono>
#include <vector>

#include "msgpack.hpp"

// nms
struct bbox {
  int classid;
  float prob;
  int xlo;
  int xhi;
  int ylo;
  int yhi;

  MSGPACK_DEFINE(classid, prob, xlo, xhi, ylo, yhi);
};

namespace demo {
namespace yolo {

struct yolov3_reply {
  std::vector<bbox> detections;

  MSGPACK_DEFINE(detections);
};

}
}
