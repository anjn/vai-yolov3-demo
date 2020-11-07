#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>

#include "zmq.hpp"
#include "msgpack.hpp"

#include "server/inf_message.hpp"
#include "utils/queue_mt.hpp"
#include "utils/sorted_queue_mt.hpp"
#include "utils/time_util.hpp"

class yolov3_client
{
  struct task {
    int frame_index;
    int frame_id;
  };

  struct task_sorter {
    constexpr bool operator()(const task &lhs, const task &rhs) const 
    {
      return lhs.frame_index > rhs.frame_index;
    }
  };

  int frame_w;
  int frame_h;
  int frame_c;

  // Frame buffer
  std::vector<std::vector<uint8_t>> frames;
  queue_mt<int> frame_ids;

  // Task
  queue_mt<task> detect_tasks;
  sorted_queue_mt<task, task_sorter> output_tasks;

  // Worker threads
  std::vector<std::thread> threads;

  // Zmq
  const std::string server;
  zmq::context_t zmq_context;

  // Label
  std::vector<std::string> class_names;
  std::vector<std::array<int,3>> class_colors;

  bool m_stop;
  mutable std::mutex m_mutex;

  int m_in_frame_index;
  int m_out_frame_index;

public:
  yolov3_client(const std::string& server_, int frame_w_, int img_h_, int frame_c_, int num_frames = 8):
    frame_w(frame_w_), frame_h(img_h_), frame_c(frame_c_),
    detect_tasks(8), output_tasks(8),
    server(server_), zmq_context(1),
    m_stop(false)
  {
    for (int i=0; i<num_frames; i++) {
      frame_ids.push(i);
      frames.emplace_back(frame_bytes());
    }
  }

  int frame_bytes() const {
    return frame_w * frame_h * frame_c;
  }

  int pop_frame_id() {
    return frame_ids.pop();
  }

  void push_frame_id(int frame_id) {
    frame_ids.push(frame_id);
  }

  uint8_t* get_frame_ptr(int frame_id) {
    return frames[frame_id].data();
  }

  void push_detect_task(int frame_id) {
    detect_tasks.push({m_in_frame_index++, frame_id});
  }

  int output_task_size() {
    return output_tasks.size();
  }

  int pop_output_task(bool sort) {
    auto id = output_tasks.pop([&](const task& t) {
      return !sort || t.frame_index == m_out_frame_index;
    }).frame_id;
    m_out_frame_index++;
    return id;
  }

  void start(int num_threads = 8)
  {
    // Load class names
    {
      std::ifstream ifs("data/voc_names.txt");
      std::string line;
      while (std::getline(ifs, line)) {
        class_names.push_back(line);
      }
    }

    // Load class colors
    {
      std::ifstream ifs("data/color_file.txt");
      std::string line;
      while (std::getline(ifs, line)) {
        int r, g, b;
        sscanf(line.c_str(), "(%d,%d,%d)", &r, &g, &b);
        class_colors.push_back({r, g, b});
      }
    }
  
    for (int i=0; i<num_threads; i++) {
      threads.emplace_back(&yolov3_client::detect, this, i);
    }
  }

  void stop()
  {
    {
      std::lock_guard<std::mutex> lk(m_mutex);
      m_stop = true;
    }

    frame_ids.stop();
    detect_tasks.stop();
    output_tasks.stop();

    for (auto& th: threads) th.join();
  }

  bool stopped()
  {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_stop;
  }

private:
  // Worker
  void detect(int worker_id)
  {
    std::cout << "Connect to " << server << std::endl;

    zmq::socket_t socket(zmq_context, ZMQ_REQ);
    socket.connect(server);

    while (true) {
      auto t = detect_tasks.pop();
      if (stopped()) break;

      // Serialize
      std::stringstream ss;
      msgpack::packer<std::stringstream> p(ss);
      p.pack_array(4);
      p.pack(frame_w);
      p.pack(frame_h);
      p.pack(frames[t.frame_id]);
      p.pack_array(1);
      p.pack("yolov3");

      // Send request
      std::cout << "Worker " << worker_id << " : Send request" << std::endl;
      zmq::message_t req(ss.str());
      socket.send(req, zmq::send_flags::none);
      
      // Get reply
      zmq::message_t rep;
      socket.recv(rep);
      std::cout << "Worker " << worker_id << " : Received reply" << std::endl;

      // Deserialize
      auto oh = msgpack::unpack(static_cast<const char*>(rep.data()), rep.size());
      auto o = oh.get();
      demo::inf_reply rep_obj;
      o.convert(rep_obj);

      // Put label text
      cv::Mat mat(frame_h, frame_w, CV_8UC4, get_frame_ptr(t.frame_id));
      for (auto& b : rep_obj.yolov3.detections) {
        auto color = class_colors[b.classid];
        auto text = class_names[b.classid];
        auto box_color = cv::Scalar(color[0], color[1], color[2], 255);
        auto font_color = cv::Scalar((color[0]+128)%256, (color[1]+128)%256, (color[2]+128)%256, 255);
        auto font_face = cv::FONT_HERSHEY_SIMPLEX;
        auto font_scale = 0.4;
        auto font_thickness = 1;
        int baseline;
        auto text_size = cv::getTextSize(text, font_face, font_scale, font_thickness, &baseline);
        int font_y = std::max(b.yhi, text_size.height + baseline + 1);
        cv::rectangle(mat, {b.xlo, b.ylo}, {b.xhi, b.yhi}, box_color, 2);
        cv::rectangle(mat, {b.xlo-1, font_y}, {b.xlo + text_size.width + 2, font_y - baseline - text_size.height}, box_color, CV_FILLED);
        cv::putText(mat, text, cv::Point(b.xlo, font_y - baseline), font_face, font_scale, font_color, font_thickness);
      }

      output_tasks.push(t);
    }
  }
};
