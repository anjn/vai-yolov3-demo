#pragma once

#include <chrono>
#include <vector>
#include <string>

struct fps_counter {
  typedef std::chrono::high_resolution_clock::time_point time_point_t;
  const int period = 60;
  std::vector<time_point_t> frame_time;
  int frame_count;
  double fps;

  fps_counter(): frame_time(period), frame_count(0), fps(0.0) {}

  double count() {
    auto last_time = frame_time[frame_count];
    auto num_frame = period;
    if (frame_time[frame_count] == time_point_t() && frame_count > 0) {
      last_time = frame_time[0];
      num_frame = frame_count + 1;
    }
    frame_time[frame_count] = std::chrono::high_resolution_clock::now();
    fps = num_frame * 1e9 / std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time[frame_count] - last_time).count();
    frame_count = (frame_count + 1) % period;
    return fps;
  }

  double get() const {
    return fps;
  }
};

struct stop_watch {
  std::chrono::high_resolution_clock::time_point start_time, last_lap;
  stop_watch() {
    start();
  }
  void start() {
    start_time = std::chrono::high_resolution_clock::now();
    last_lap = start_time;
  }
  void lap(std::string name) {
    auto time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(time - last_lap).count();
    std::cout << name << " : " << (duration / 1e6) << " ms" << std::endl;
    last_lap = time;
  }
  void time(std::string name) {
    auto time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(time - start_time).count();
    std::cout << name << " : " << (duration / 1e6) << " ms" << std::endl;
  }
  int get_time_ns() {
    auto time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time - start_time).count();
  }
};

