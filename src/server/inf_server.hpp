#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <sstream>
#include <mutex>

#include "msgpack.hpp"
#include "zmq.hpp"

#include "server/inf_message.hpp"
#include "server/inf_monitor.hpp"
#include "utils/time_util.hpp"

#include "server/yolo/yolov3_model.hpp"

namespace demo {

struct inf_server_config
{
  std::string xclbin;
  std::vector<inf_model_config> model_confs;
  std::string address;
  int num_threads;
};

class inf_server
{
  // xDNN
  const inf_server_config& conf;
  std::map<std::string, std::unique_ptr<inf_model_base>> models;

  // Worker threads
  std::vector<std::thread> threads;

  // Zmq
  const std::string address;
  zmq::context_t zmq_context;

  bool m_stop;
  mutable std::mutex m_mutex;

public:
  inf_server(const inf_server_config& conf_)
  : conf(conf_),
    zmq_context(1),
    m_stop(false)
  {}

  void start()
  {
    // Register models
    for (auto& c: conf.model_confs) {
      if (c.name == "yolov3") {
        models[ec.name] = std::make_unique<yolo::yolov3_model>(c);
      }
    }

    // Start worker threads
    std::cout << "Num worker threads: " << conf.num_threads << std::endl;
    for (int i=0; i<conf.num_threads; i++) {
      threads.emplace_back(&inf_server::infer, this);
    }

    // Start server
    std::cout << "Starting server " << conf.address << std::endl;
    std::cout << std::endl;
    std::cout << "Ctrl-C to stop" << std::endl;
    zmq::socket_t clients(zmq_context, ZMQ_ROUTER);
    clients.bind(conf.address);

    zmq::socket_t workers(zmq_context, ZMQ_DEALER);
    workers.bind("inproc://workers");

    // Start monitor
    inf_monitor monitor;
    auto monitor_th = std::thread(&inf_monitor::monitor, &monitor);

    zmq::proxy(clients, workers);
  }

  void stop() {
    {
      std::lock_guard<std::mutex> lk(m_mutex);
      m_stop = true;
    }
    for (auto& th: threads) th.join();
  }

  bool stopped() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_stop;
  }

private:

  // Worker
  void infer()
  {
    //std::cout << "Start worker thread" << std::endl;
    zmq::socket_t socket(zmq_context, ZMQ_REP);
    socket.connect("inproc://workers");

    zmq::socket_t monitor(zmq_context, ZMQ_PUSH);
    monitor.connect("tcp://localhost:5558");

    while (true) {
      // Get request
      zmq::message_t req;
      socket.recv(req);

      // Deserialize
      inf_request req_obj;

      auto oh = msgpack::unpack(static_cast<const char*>(req.data()), req.size());
      auto o = oh.get();

      try
      {
        msgpack::type::tuple<int, int, std::vector<uint8_t>,
          std::vector<std::string>> tmp;
        o.convert(tmp);
  
        req_obj.image_w = tmp.get<0>();
        req_obj.image_h = tmp.get<1>();
        req_obj.image = std::move(tmp.get<2>());
        req_obj.inferences = std::move(tmp.get<3>());
      }
      catch (...)
      {
        // For Python clients
        msgpack::type::tuple<int, int, std::vector<int>,
          std::vector<std::string>> tmp;
        o.convert(tmp);

        req_obj.image_w = tmp.get<0>();
        req_obj.image_h = tmp.get<1>();
        auto& arr = tmp.get<2>();
        auto size = arr.size();
        req_obj.image.resize(size);
        for (int i=0; i<size; i++) req_obj.image[i] = arr[i];
        req_obj.inferences = tmp.get<3>();
      }

      inf_reply rep_obj;

      // Run
      for (auto& inf: req_obj.inferences) {
        auto it = models.find(inf);
        if (it != models.end()) {
          auto& model = it->second;
          stop_watch sw;
          model->infer(req_obj, rep_obj);

          inf_event ev;
          ev.float_params[model->name + "_latency"] = float(sw.get_time_ns())/1e6;
          ev.send(monitor);
        }
      }

      // Serialize
      std::stringstream ss;
      msgpack::pack(ss, rep_obj);

      // Send reply
      zmq::message_t rep(ss.str());
      socket.send(rep, zmq::send_flags::none);
    }
  }

};

}
