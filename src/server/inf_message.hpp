#pragma once
#include <vector>
#include <string>

#include "msgpack.hpp"

#include "server/yolo/yolov3_message.hpp"

namespace demo {

struct inf_request
{
  int image_w;
  int image_h;
  std::vector<uint8_t> image;
  std::vector<std::string> inferences;

  void unpack(const char* data, size_t size) {
    auto oh = msgpack::unpack(data, size);
    auto o = oh.get();

    try
    {
      msgpack::type::tuple<int, int, std::vector<uint8_t>, std::vector<std::string>> tmp;
      o.convert(tmp);

      image_w = tmp.get<0>();
      image_h = tmp.get<1>();
      image = std::move(tmp.get<2>());
      inferences = std::move(tmp.get<3>());
    }
    catch (...)
    {
      // For Python clients
      msgpack::type::tuple<int, int, std::vector<int>,
        std::vector<std::string>> tmp;
      o.convert(tmp);

      image_w = tmp.get<0>();
      image_h = tmp.get<1>();
      auto& arr = tmp.get<2>();
      auto size = arr.size();
      image.resize(size);
      for (int i=0; i<size; i++) image[i] = arr[i];
      inferences = tmp.get<3>();
    }
  }

  MSGPACK_DEFINE(image_w, image_h, image, inferences);
};

struct inf_reply
{
  yolo::yolov3_reply yolov3;

  MSGPACK_DEFINE(yolov3);
};

struct inf_request_batch
{
  std::vector<inf_request> reqs;

  MSGPACK_DEFINE(reqs);
};

}

