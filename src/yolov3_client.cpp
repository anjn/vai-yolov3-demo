#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "arg/arg.h"
#include "client/yolov3_client.hpp"

int main(int argc, char** argv)
{
  // Parse arguments
  arg_begin("RAW_VIDEO WIDTH HEIGHT", 3, 3);
  arg_i(num_threads, 8, "The number of detection threads");
  arg_s(server, "tcp://localhost:5555", "Server address");
  arg_i(window_width, -1, "Window width (default: same with video size)");
  arg_i(display_fps, 30, "Display frame per second");
  arg_b(normal, false, "Play at normal speed");
  arg_end;

  const auto video_file = args[0];
  const int img_w = args.as<int>(1);
  const int img_h = args.as<int>(2);
  const int img_c = 4;

  yolov3_client client(server, img_w, img_h, img_c);
  client.start(num_threads);

  // OpenGL Init
  if (glfwInit() == GL_FALSE) {
    std::cerr << "Error" << std::endl;
    return 1;
  }

  atexit(glfwTerminate);

  const int win_w = window_width <= 0 ? img_w : window_width;
  const int win_h = win_w * img_h / img_w;

  using namespace std::literals::string_literals;
  auto window = glfwCreateWindow(win_w, win_h, ("YOLOv3 416x416, 20 classes"s).c_str(), NULL, NULL);
  if (window == nullptr) {
    std::cerr << "Error" << std::endl;
    return 1;
  }

  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK) {
    std::cerr << "Error" << std::endl;
    return 1;
  }

  glfwSwapInterval(1);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  glClearColor(1.0f, 0.8f, 1.0f, 1.0f);

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img_w, img_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  GLuint readFboId;
  glGenFramebuffers(1, &readFboId);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId);
  glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  printf("\n");
  printf("Press ESC to exit\n");
  printf("\n");

  // Launch threads
  std::vector<std::thread> threads;

  // Video stream
  bool video_stop = false;
  threads.emplace_back([&] {
    FILE* fp;
    if (video_file == "-") {
      fp = stdin;
    } else {
      fp = fopen(video_file.c_str(), "rb");
      if (fp == nullptr)
        printf("Error: file not found! '%s'\n", video_file.c_str());
    }

    const auto frame_bytes = client.frame_bytes();

    while (true) {
      int frame_id = client.pop_frame_id();
      if (client.stopped()) break;

      auto frame = client.get_frame_ptr(frame_id);

      auto s = fread(frame, 1, frame_bytes, fp);
      if (s < frame_bytes) break;

      client.push_detect_task(frame_id);
    }

    video_stop = true;
  });

  int frame_id = -1;
  fps_counter fps;

  int display_period = 1e9/display_fps;
  stop_watch display_sw;

  auto get_next_frame = [&] {
    if (frame_id >= 0) client.push_frame_id(frame_id);
    frame_id = client.pop_output_task(true);
    fps.count();
  };

  // Display loop
  while (glfwWindowShouldClose(window) == GL_FALSE)
  {
    // Get next frame
    if (!normal || frame_id < 0)
      get_next_frame();

    // Get latest frame
    while (!normal && client.output_task_size() > 0) {
      get_next_frame();
    }

    if (display_sw.get_time_ns() >= display_period) {
      display_sw.start();

      auto frame = client.get_frame_ptr(frame_id);
  
      // Draw fps
      cv::Mat mat(img_h, img_w, CV_8UC4, frame);
      std::ostringstream oss;
      oss << "FPS: " << std::fixed << std::setprecision(2) << fps.get();
      cv::putText(mat, oss.str(), cv::Point(5,20), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0,255,0,255), 2);
      cv::flip(mat, mat, 0);
      //printf("fps: %.2lf\n", fps.get());
    
      // Draw frame to window
      glClear(GL_COLOR_BUFFER_BIT);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img_w, img_h, GL_BGRA, GL_UNSIGNED_BYTE, frame);
  
      glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId);
      glBlitFramebuffer(0, 0, img_w, img_h,
          0, 0, win_w, win_h,
          GL_COLOR_BUFFER_BIT, GL_LINEAR);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  
      glfwSwapBuffers(window);

      client.push_frame_id(frame_id);
      frame_id = -1; 

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  
    // Check window close
    glfwPollEvents();

    // Check stop
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || video_stop)
      glfwSetWindowShouldClose(window, true);
  }

  // Release
  client.stop();
}


