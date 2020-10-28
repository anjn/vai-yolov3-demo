#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include "arg/arg.h"

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

  cv::Mat frame;
  cap >> frame;
  cap.release();

  std::cout << "width=" << frame.cols << "; height=" << frame.rows;
}
