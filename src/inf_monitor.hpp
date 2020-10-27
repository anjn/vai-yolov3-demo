#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>

#include "msgpack.hpp"
#include "termbox.h"

namespace demo {

struct inf_event
{
  std::map<std::string, float> float_params;

  void send(zmq::socket_t& socket)
  {
    std::stringstream ss;
    msgpack::pack(ss, *this);
    zmq::message_t m(ss.str());
    socket.send(m, zmq::send_flags::none);
  }

  void recv(zmq::socket_t& socket)
  {
    zmq::message_t m;
    socket.recv(m);

    auto oh = msgpack::unpack(static_cast<const char*>(m.data()), m.size());
    auto o = oh.get();
    o.convert(*this);
  }

  MSGPACK_DEFINE(float_params);
};

struct inf_status
{
  float yolov3_latency { 0.0f };

  int yolov3_count { 0 };

  void update(const xdnn_event& ev) {
    for (const auto& p: ev.float_params) {
      //std::cout << "event: " << p.first << " = " << p.second << std::endl;
      if (p.first == "yolov3_latency") {
        yolov3_latency = p.second;
        yolov3_count++;
      }
    }
  }
};

struct inf_monitor
{
  mutable std::mutex m_mutex;
  inf_status status;

  inf_status get_status() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return status;
  }

  void subscribe() {
    zmq::context_t zmq_context(1);
    zmq::socket_t socket(zmq_context, ZMQ_PULL);
    socket.bind("tcp://*:5558");

    while (true)
    {
      inf_event ev;
      ev.recv(socket);
      {
        std::lock_guard<std::mutex> lk(m_mutex);
        status.update(ev);
      }
    }
  }

  void monitor()
  {
    std::thread sub_th(&inf_monitor::subscribe, this);

    int w = 0;
    int h = 0;
    int code = tb_init();
    if (code < 0) {
      fprintf(stderr, "termbox init failed, code: %d\n", code);
      goto finish;
    }
  
    tb_select_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
  
    w = tb_width();
    h = tb_height();
    update(-1, -1);
    for (;;) {
      struct tb_event ev;
      int mx = -1;
      int my = -1;
      int t = tb_peek_event(&ev, 100);
      if (t == -1) {
        tb_shutdown();
        fprintf(stderr, "termbox poll event error\n");
        goto finish;
      }
  
      switch (t) {
        case TB_EVENT_KEY:
          if (ev.key == TB_KEY_ESC) {
            tb_shutdown();
            goto finish;
          }
          break;
        case TB_EVENT_MOUSE:
          if (ev.key == TB_KEY_MOUSE_LEFT) {
            mx = ev.x;
            my = ev.y;
          }
          break;
        case TB_EVENT_RESIZE:
          break;
      }

      update(mx, my);
    }

finish:
    sub_th.join();
  }

  void update(int mx, int my) {
    //device.update();
    auto s = get_status();

    tb_clear();
    uint16_t fg = TB_RED;
    uint16_t bg = TB_DEFAULT;
    //draw_button(0,1,"Test!");

    std::stringstream ss;
    ss << "Press [ESC] to exit\n\n";
    ss << "YOLOv2 latency: " << s.yolov3_latency << " ms\n";
    ss << "YOLOv2 count: " << s.yolov3_count << "\n";
    //ss << "Power: " << device.get_power() << " W\n";
    //ss << "FPGA Temp: " << device.get_fpga_temp() << " C\n";
    //ss << "Fan speed: " << device.get_fan_rpm() << " RPM\n";
    draw_text(0, 0, ss.str());

    tb_present();
  }

  const std::vector<uint32_t> button_border {
    0x256d,
    0x2500,
    0x256e,
    0x2502,
    0x0,
    0x2502,
    0x2570,
    0x2500,
    0x256f,
  };

  void draw_text(int x, int y, const std::string& text)
  {
    uint16_t fg = TB_RED;
    uint16_t bg = TB_DEFAULT;
    int tx = x;
    int ty = y;
    for (int i=0; i<text.size(); i++) {
      if (text[i] == '\n') {
        tx = x;
        ty++;
      } else {
        tb_change_cell(tx++, ty, text[i], fg, bg);
      }
    }
  }
  
  void draw_button(int x, int y, const std::string& text, int padding_left = 0, int padding_right = 0)
  {
    uint16_t fg = TB_RED;
    uint16_t bg = TB_DEFAULT;
    int btn_w = padding_left + padding_right + text.size();
  
    tb_change_cell(x, y, button_border[0], fg, bg);
    for (int i=0; i<btn_w; i++) tb_change_cell(x+1+i, y, button_border[1], fg, bg);
    tb_change_cell(x+1+btn_w, y, button_border[2], fg, bg);
  
    tb_change_cell(x, y+1, button_border[3], fg, bg);
    for (int i=0; i<padding_left; i++) tb_change_cell(x+1+i, y+1, ' ', fg, bg);
    for (int i=0; i<text.size(); i++) tb_change_cell(x+1+padding_left+i, y+1, text[i], fg, bg);
    for (int i=0; i<padding_right; i++) tb_change_cell(x+1+padding_left+text.size()+i, y+1, ' ', fg, bg);
    tb_change_cell(x+1+btn_w, y+1, button_border[5], fg, bg);
  
    tb_change_cell(x, y+2, button_border[6], fg, bg);
    for (int i=0; i<btn_w; i++) tb_change_cell(x+1+i, y+2, button_border[7], fg, bg);
    tb_change_cell(x+1+btn_w, y+2, button_border[8], fg, bg);
  }

};


}
