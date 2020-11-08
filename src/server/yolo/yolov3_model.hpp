#pragma once
#include <opencv2/opencv.hpp>

#include "server/inf_message.hpp"
#include "server/inf_model_base.hpp"
#include "server/inf_model_config.hpp"
#include "utils/time_util.hpp"

namespace demo {
namespace yolo {

inline const int yolov3_img_w = 416;
inline const int yolov3_img_h = 416;
inline const int anchor_boxes = 3;
inline const int classes      = 20;

//inline const int yolov3_img_w = 512;
//inline const int yolov3_img_h = 256;
//inline const int anchor_boxes = 5;
//inline const int classes      = 3;

inline const float confidence = 0.5;

struct yolov3_model : public inf_model_base
{
  const int input_width = yolov3_img_w;
  const int input_height = yolov3_img_h;

  std::vector<float> biases;
  std::vector<std::string> labels;

  yolov3_model(const inf_model_config& conf, const xir::Subgraph* subgraph)
  : inf_model_base::inf_model_base(conf, subgraph)
  {
    biases = {
      116,90, 156,198, 373,326,
      30,61, 62,45, 59,119,
      10,13, 16,30, 33,23
    };
    labels = {
      "aeroplane","bicycle","bird","boat","bottle",
      "bus","car","cat","chair","cow","diningtable",
      "dog","horse","motorbike","person","pottedplant",
      "sheep","sofa","train","tvmonitor"
    };

    //biases = { 123,100, 167,83, 98,174, 165,158, 347,98, 76,37,
    //           40,97, 74,64, 105,63, 66,131,18,46, 33,29, 47,23,
    //           28,68, 52,42, 5.5,7, 8,17, 14,11, 13,29, 24,17 };
    //labels = {"car","person","cycle"};
  }

  void infer(std::vector<inf_request>& reqs, std::vector<inf_reply>& reps) override
  {
    const int frame_c = 4;
    const auto frame_w = reqs[0].image_w;
    const auto frame_h = reqs[0].image_h;

    const int batch = reqs.size();

    // Preprocess
    for (int i=0; i<batch; i++) {
      const auto frame_ptr = reqs[i].image.data();

      if (frame_w == input_width) {
        preprocess_input(frame_ptr, frame_w, frame_h, frame_c, buf.buffers["input0"].data() + input_width * input_height * 3 * i);
      } else {
        assert(false);
        // Resize
        int img_w = input_width;
        int img_h = std::min(input_width * frame_h / frame_w, input_height);
        std::cout << "Resize : from " << frame_w << "x" << frame_h << ", to " << img_w << "x" << img_h << std::endl;
        cv::Mat src(frame_w, frame_h, CV_8UC4, frame_ptr);
        cv::Mat dst(img_w, img_h, CV_8UC4);
        cv::resize(src, dst, cv::Size(img_w, img_h));
        preprocess_input(dst.data, img_w, img_h, frame_c, buf.buffers["input0"].data() + input_width * input_height * 3 * i);
      }
    }
    
    // Submit inference
    execute();

    reps.resize(batch);

    // Postprocess
    for (int bi=0; bi<batch; bi++) {
      auto& rep = reps[bi];

      std::vector<std::vector<float>> boxes;
      std::vector<int> scale_feature;

      for (auto s: buf.output_shapes) scale_feature.push_back(s[2]);
      std::sort(scale_feature.begin(), scale_feature.end(), [](int a, int b){return a < b;});

      const int num_outputs = buf.output_ptrs.size();

      for (auto i = 0; i < num_outputs; i++) {
        const auto result = buf.output_raw_buffers[i];
        const auto shape = buf.output_shapes[i];
        const int height = shape[1];
        const int width = shape[2];
        const int channles = shape[3];

        int index = 0;
        for (int j = 0; j < num_outputs; j++) {
          if (width == scale_feature[j]) {
            index = j;
            break;
          }
        }

        //std::cout << "output width " << width << ", index " << index << std::endl;

        // swap the network's result to a new sequence
        const int conf_box = 5 + classes;
        float swap[height][width][anchor_boxes][conf_box];
        for (int y = 0; y < height; y++)
          for (int x = 0; x < width; x++)
            for (int c = 0 ; c < channles; c++)
              swap[y][x][c/conf_box][c%conf_box] = 
                result[height * width * channles * bi + y * width * channles + x * channles + c];

        // compute the coordinate of each primary box
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
            for (int a = 0 ; a < anchor_boxes; a++) {
              float obj_score = sigmoid(swap[y][x][a][4]);
              if(obj_score < confidence)
                continue;

              std::vector<float> box;
              float xx = (x + sigmoid(swap[y][x][a][0])) / width;
              float yy = (y + sigmoid(swap[y][x][a][1])) / height;
              float ww = exp(swap[y][x][a][2]) * biases.at((anchor_boxes * index + a) * 2 + 0) / input_width;
              float hh = exp(swap[y][x][a][3]) * biases.at((anchor_boxes * index + a) * 2 + 1) / input_height;
              box.push_back(xx);
              box.push_back(yy);
              box.push_back(ww);
              box.push_back(hh);
              box.push_back(-1);
              box.push_back(obj_score);
              for(int p =0; p < classes; p++)
                box.push_back(obj_score * sigmoid(swap[y][x][a][5 + p]));
              boxes.push_back(box);
            }
          }
        }
      }

      boxes = correct_region_boxes(boxes, boxes.size(), frame_w, frame_h, input_width, input_height);

      vector<vector<float>> res = apply_nms(boxes, classes, 0.3);

      for (auto& r: res) {
        bbox b;
        b.xlo = std::min(frame_w-1, std::max(0, (int) ((r[0] - r[2] / 2.0) * frame_w)));
        b.ylo = std::min(frame_h-1, std::max(0, (int) ((r[1] - r[3] / 2.0) * frame_h)));
        b.xhi = std::min(frame_w-1, std::max(0, (int) ((r[0] + r[2] / 2.0) * frame_w)));
        b.yhi = std::min(frame_h-1, std::max(0, (int) ((r[1] + r[3] / 2.0) * frame_h)));
        b.classid = r[4];
        b.prob = r[6 + b.classid];

        rep.yolov3.detections.push_back(b);
      }
    }
  }

  inline float sigmoid(float v) const {
    if (v >= 0.0f) {
      return 1.0f / (1.0f + expf(-v));
    } else {
      auto e = expf(v);
      return e / (1.0f + e);
    }
  }

  inline float overlap(float x1, float w1, float x2, float w2) const {
    float left = max(x1 - w1 / 2.0, x2 - w2 / 2.0);
    float right = min(x1 + w1 / 2.0, x2 + w2 / 2.0);
    return right - left;
  }
  
  inline float cal_iou(const std::vector<float>& box, const std::vector<float>& truth) const {
    float w = overlap(box[0], box[2], truth[0], truth[2]);
    float h = overlap(box[1], box[3], truth[1], truth[3]);
    if (w<0 || h<0) return 0;
    float inter_area = w * h;
    float union_area = box[2] * box[3] + truth[2] * truth[3] - inter_area;
    return inter_area * 1.0 / union_area;
  }

  std::vector<std::vector<float>>
  apply_nms(std::vector<std::vector<float>>& boxes, int classes, float thres) const {
    stop_watch sw;
    //const float confidence = 0.005;
    std::vector<pair<int, float>> order(boxes.size());
    std::vector<std::vector<float>> result;
    for (int k = 0; k < classes; k++) {
      for (size_t i = 0; i < boxes.size(); ++i) {
        order[i].first = i;
        boxes[i][4] = k;
        order[i].second = boxes[i][6+k];
      }
      sort(order.begin(), order.end(),[](const pair<int, float>& ls, const pair<int, float>& rs) {return ls.second > rs.second;});
      std::vector<bool> exist_box(boxes.size(), true);
      for (size_t _i = 0; _i < boxes.size(); ++_i) {
        size_t i = order[_i].first;
        if (!exist_box[i]) continue;
        if (boxes[i][6 + k] < confidence) {
          exist_box[i] = false;
          //break;
          continue;
        }
        result.push_back(boxes[i]);
        for (size_t _j = _i+1; _j < boxes.size(); ++_j) {
          size_t j = order[_j].first;
          if (!exist_box[j]) continue;
          float ovr = cal_iou(boxes[j], boxes[i]);
          //std::cout << "iou : " << ovr << std::endl;
          if (ovr >= thres) exist_box[j] = false;
        }
      }
      //std::cout << k << ", " << result.size() << std::endl;
    }
    //sw.time("apply_nms");
    //std::cout << "boxes after nms: " << result.size() << std::endl;
    return result;
  }

  std::vector<std::vector<float>>
  correct_region_boxes(std::vector<std::vector<float>> boxes, int n, int w, int h, int netw, int neth) const {
    int new_w = 0;
    int new_h = 0;
    if (((float)netw / w) < ((float)neth / h)) {
      new_w = netw;
      new_h = (h * netw) / w;
    } else {
      new_w = (w * neth) / h;
      new_h = neth;
    }
    for (int i = 0; i < boxes.size() ; i++) {
      boxes[i][0] = (boxes[i][0] - (netw - new_w) / 2.0 / netw) / ((float)new_w / netw);
      boxes[i][1] = (boxes[i][1] - (neth - new_h) / 2.0 / neth) / ((float)new_h / neth);
      boxes[i][2] *= (float)netw / new_w;
      boxes[i][3] *= (float)neth / new_h;
    }
    return boxes;
  }

  void preprocess_input(uint8_t* img, int img_w, int img_h, int img_c, float* output) const
  {
    const int out_w = input_width;
    const int out_h = input_height;

    assert(img_w == out_w);
  
    const int start_y = (out_h - img_h) / 2;

    float scale = 1.0f/256.0f;
  
    int n = out_w * start_y * 3;
    for (int i=0; i<n; i++) {
      *output++ = 0.5f;
    }
  
    n = out_w * img_h;
    for (int i=0; i<n; i++) {
      *output++ = img[0] * scale;
      *output++ = img[1] * scale;
      *output++ = img[2] * scale;
      img += img_c;
    }
  
    n = out_w * (out_h - start_y - img_h) * 3;
    for (int i=0; i<n; i++) {
      *output++ = 0.5f;
    }
  }
};

}
}
