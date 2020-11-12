#include <cstdlib>
#include <algorithm>
#include <random>
#include <iostream>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include "arg/arg.h"

using namespace cv;

int main(int argc, char** argv)
{
  arg_begin("IMG_LIST", 1, 1);
  arg_b(random_order, true);
  arg_end;

  std::vector<std::string> files;
  {
    std::ifstream ifs(args[0]);
    std::string line;
    while (std::getline(ifs, line)) {
      files.push_back(line);
    }
  }

  if (random_order) {
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::shuffle(files.begin(), files.end(), engine);
  }

  const int out_w = 416;
  const int out_h = 416;

  cv::Mat out_c3(out_w, out_h, CV_8UC3);
  cv::Mat out_c4(out_w, out_h, CV_8UC4);

  for (auto& f: files) {
    Mat frame = imread(f);
    resize(frame, out_c3, Size(out_w, out_h));
    cvtColor(out_c3, out_c4, CV_BGR2BGRA);
    fwrite(out_c4.data, 1, out_w*out_h*4, stdout);
  }
}

