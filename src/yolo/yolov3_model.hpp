#pragma once
#include <opencv2/opencv.hpp>

#include "inf_model_base.hpp"

extern "C" {

void free_bboxes(bbox *arr);

int do_nms(float *arr, int cnt,
	   int im_w, int im_h,
	   int net_w, int net_h,
	   int out_w, int out_h,
	   int bboxplanes,
	   int classes,
	   float scoreThreshold, 
	   float iouThreshold,
	   int *numBoxes, bbox **bboxes);

} // extern "C"

namespace mls {
namespace yolo {

struct yolov3_model : public inf_model_base
{
  yolov3_model(const xdnn_config& conf)
  : inf_model_base::inf_model_base(conf)
  {}

  void infer(inf_request& req, inf_reply& rep) override
  {
    auto stream_id = pe.pop_stream_id();
    auto& inout = pe.get_buffer(stream_id);
    auto& output = inout.buffers["output"];

    int frame_c = 4;
    auto frame_w = req.image_w;
    auto frame_h = req.image_h;
    auto frame_ptr = req.image.data();

    // Preprocess
    if (frame_w == 608) {
      preprocess_input(frame_ptr, frame_w, frame_h, frame_c, inout.buffers["input"].data());
    } else {
      // Resize
      int img_w = 608;
      int img_h = 608 * frame_h / frame_w;
      cv::Mat src(frame_w, frame_h, CV_8UC4, frame_ptr);
      cv::Mat dst(img_w, img_h, CV_8UC4);
      cv::resize(src, dst, cv::Size(img_w, img_h));
      preprocess_input(dst.data, img_w, img_h, frame_c, inout.buffers["input"].data());
    }

    // Submit inference
    pe.execute(stream_id);

    // Postprocess
    const int anchor_boxes = 5;
    const int objectness   = 1;
    const int coordinates  = 4;
    const int classes      = 80;
    const int out_w        = 608 / 32;
    const int out_h        = 608 / 32;
    const int out_c        = coordinates + objectness + classes;

    // Apply sigmoid to X,Y,Objectness
    {
      auto channels = {0, 1, 4};
      for (int a=0; a<anchor_boxes; a++) {
        for (auto c: channels) {
          for (int y=0; y<out_h; y++) {
            for (int x=0; x<out_w; x++) {
              int i = out_c*out_h*out_w*a + out_h*out_w*c + out_w*y + x;
              output[i] = sigmoid(output[i]);
            }
          }
        }
      }
    }
  
    // Softmax
    for (int a=0; a<anchor_boxes; a++) {
      const int i0 = out_w*out_h*(out_c*a + 5);
      float max_v = std::numeric_limits<float>::min();
      for (int c=0; c<classes; c++) {
        for (int i=0, n=out_h*out_w; i<n; i++) {
          max_v = std::max(output[i0 + out_h*out_w*c + i], max_v);
        }
      }
      for (int i=0, n=out_h*out_w; i<n; i++) {
        float sum_v = 0.0f;
        for (int c=0; c<classes; c++) {
          int j = i0 + out_h*out_w*c + i;
          output[j] = expf(output[j] - max_v);
          sum_v += output[j];
        }
        for (int c=0; c<classes; c++) {
          int j = i0 + out_h*out_w*c + i;
          output[j] = output[j] / sum_v;
        }
      }
    }
  
    // NMS
    float scoreThreshold = 0.24f;
    float iouThreshold = 0.3f;
    int numBoxes;
    bbox *bboxes;
    do_nms(output.data(), output.size(), frame_w, frame_h, 608, 608,
      out_w, out_h, anchor_boxes, classes, scoreThreshold, iouThreshold, &numBoxes, &bboxes);

    for (int i=0; i<numBoxes; i++) {
      rep.yolov2.detections.push_back(bboxes[i]);
    }

    free_bboxes(bboxes);

    // Release stream
    pe.push_stream_id(stream_id);
  }

  float sigmoid(float v) const {
    if (v >= 0.0f) {
      return 1.0f / (1.0f + expf(-v));
    } else {
      auto e = expf(v);
      return e / (1.0f + e);
    }
  }

  void preprocess_input(uint8_t* img, int img_w, int img_h, int img_c, float* output) const
  {
    const int out_w = 608;
    const int out_h = 608;
  
    // assert(img_w == out_w);
  
    const int start_y = (out_h - img_h) / 2;
  
    float* output_b = output + out_w * out_h * 0;
    float* output_g = output + out_w * out_h * 1;
    float* output_r = output + out_w * out_h * 2;
  
    float scale = 1.0f/255.0f;
  
    int n = out_w*start_y;
    for (int i=0; i<n; i++) {
      *(output_r++) = 0.5f;
      *(output_g++) = 0.5f;
      *(output_b++) = 0.5f;
    }
  
    n = out_w*img_h;
    for (int i=0; i<n; i++) {
      *(output_r++) = img[0] * scale;
      *(output_g++) = img[1] * scale;
      *(output_b++) = img[2] * scale;
      img += img_c;
    }
  
    n = out_w*(out_h-start_y-img_h);
    for (int i=0; i<n; i++) {
      *(output_r++) = 0.5f;
      *(output_g++) = 0.5f;
      *(output_b++) = 0.5f;
    }
  }
};

}
}
