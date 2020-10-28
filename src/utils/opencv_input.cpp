#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include "arg/arg.h"

using namespace cv;

int main(int argc, char** argv)
{
  arg_begin("INPUT", 1, 1);
  arg_end;

  cv::VideoCapture cap;
  try {
    cap.open(argv[1]);
    if (!cap.isOpened()) {
       cap.open(args.as<int>(0));
    }
    if (!cap.isOpened()) {
      throw std::runtime_error("");
    }
  } catch (...) {
    std::cerr << "Couldn't open the input!" << std::endl;
    exit(EXIT_FAILURE);
  }

  Mat frame;
  cap >> frame;

  const int out_w = 608;
  const int out_h = 608 * frame.rows / frame.cols;

  cv::Mat out_c3(out_w, out_h, CV_8UC3);
  cv::Mat out_c4(out_w, out_h, CV_8UC4);

  while (true)
  {
    cap >> frame;
    resize(frame, out_c3, Size(out_w, out_h));
    cvtColor(out_c3, out_c4, CV_BGR2BGRA);
    fwrite(out_c4.data, 1, out_w*out_h*4, stdout);
  }
}
