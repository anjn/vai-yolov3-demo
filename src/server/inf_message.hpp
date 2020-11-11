#pragma once
#include <vector>
#include <string>

#include "msgpack.hpp"

#include "server/yolo/yolov3_message.hpp"

namespace demo {

struct inf_request
{
  uint64_t id;
  int image_w;
  int image_h;
  std::vector<uint8_t> image;
  std::vector<std::string> inferences;

  void unpack(const char* data, size_t size) {
    auto oh = msgpack::unpack(data, size);
    auto o = oh.get();

    try
    {
      msgpack::type::tuple<uint64_t, int, int, std::vector<uint8_t>, std::vector<std::string>> tmp;
      o.convert(tmp);

      id = tmp.get<0>();
      image_w = tmp.get<1>();
      image_h = tmp.get<2>();
      image = std::move(tmp.get<3>());
      inferences = std::move(tmp.get<4>());
    }
    catch (...)
    {
      // For Python clients
      msgpack::type::tuple<uint64_t, int, int, std::vector<int>, std::vector<std::string>> tmp;
      o.convert(tmp);

      id = tmp.get<0>();
      image_w = tmp.get<1>();
      image_h = tmp.get<2>();
      auto& arr = tmp.get<3>();
      auto size = arr.size();
      image.resize(size);
      for (int i=0; i<size; i++) image[i] = arr[i];
      inferences = tmp.get<4>();
    }
  }

  MSGPACK_DEFINE(id, image_w, image_h, image, inferences);
};

struct inf_reply
{
  uint64_t req_id;
  yolo::yolov3_reply yolov3;

  MSGPACK_DEFINE(req_id, yolov3);
};

struct inf_request_batch
{
  std::vector<inf_request> reqs;

  MSGPACK_DEFINE(reqs);
};

}

