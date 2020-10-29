#include <opencv2/opencv.hpp>

#include "arg/arg.h"

#include "server/yolo/yolov3_message.hpp"
#include "server/yolo/yolov3_model.hpp"

int main(int argc, char** argv)
{
  // Parse arguments
  arg_begin("INPUT", 1, 1);
  arg_end;

  demo::inf_model_config conf;
  conf.name = "yolov3";
  conf.xmodel = "model/yolov3.xmodel";
  //conf.xmodel = "model/yolov3_adas_pruned_0_9.xmodel";
  conf.num_runners = 1;
  conf.num_streams = 1;

  demo::inf_request req;
  demo::inf_reply rep;

  demo::yolo::yolov3_model model(conf);

  auto img = cv::imread(args[0]);

    const int out_w = model.input_width;
    const int out_h = std::min(model.input_width * img.rows / img.cols, model.input_height);
  {
    //const int out_w = img.cols;
    //const int out_h = img.rows;
    cv::Mat out_c3(out_w, out_h, CV_8UC3);
    cv::Mat out_c4(out_w, out_h, CV_8UC4);
    cv::resize(img, out_c3, cv::Size(out_w, out_h));
    cv::cvtColor(out_c3, out_c4, CV_BGR2BGRA);

    const int size = out_w * out_h * 4;
    req.image_w = out_w;
    req.image_h = out_h;
    req.image.resize(size);
    std::memcpy(req.image.data(), out_c4.data, size);
    req.inferences.push_back("yolov3");
  }

  model.infer(req, rep);

  for (auto& b: rep.yolov3.detections) {
    auto text = model.labels[b.classid];
    b.xlo = b.xlo * img.cols / out_w;
    b.xhi = b.xhi * img.cols / out_w;
    b.ylo = b.ylo * img.rows / out_h;
    b.yhi = b.yhi * img.rows / out_h;
    //std::cout << "class : " << text << std::endl;
    cv::rectangle(img, {b.xlo, b.ylo}, {b.xhi, b.yhi}, cv::Scalar(255,0,0), 2);
    cv::putText(img, text, cv::Point(b.xlo, b.ylo), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,0,0), 1);
  }

  cv::imshow("image", img);
  cv::waitKey(0);
}


