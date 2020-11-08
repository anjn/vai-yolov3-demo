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
#include "server/inf_dynamic_batching.hpp"
#include "utils/queue_mt.hpp"
#include "utils/time_util.hpp"

#include "server/yolo/yolov3_model.hpp"

namespace demo {

struct inf_server_config
{
  std::string address;
  int max_batch_size;
  int max_batch_latency;
  std::vector<inf_model_config> model_confs;
};

class inf_server
{
  std::map<std::string, std::unique_ptr<xir::Graph>> graphs;
  std::map<std::string, std::vector<const xir::Subgraph*>> subgraphs;

  const inf_server_config& conf;
  //std::map<std::string, std::unique_ptr<inf_model_base>> models;
  std::vector<std::shared_ptr<inf_model_base>> models;

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
    // Create models
    for (auto& c: conf.model_confs) {
      graphs[c.name] = xir::Graph::deserialize(c.xmodel);
      subgraphs[c.name] = get_dpu_subgraph(graphs[c.name].get());
      std::cout << "Num worker threads: " << c.num_workers << std::endl;
      for (int i=0; i<c.num_workers; i++) {
        if (c.name == "yolov3") {
          auto model = std::make_shared<yolo::yolov3_model>(c, subgraphs[c.name][0]);
          models.push_back(model);
          threads.emplace_back(&inf_server::infer, this, i, model);
        }
      }
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
    //inf_monitor monitor;
    //auto monitor_th = std::thread(&inf_monitor::monitor, &monitor);

    //zmq::proxy(clients, workers);
    dynamic_batching(clients, workers, conf.max_batch_size, conf.max_batch_latency);
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
  void infer(int worker_id, std::shared_ptr<inf_model_base> model)
  {
    //std::cout << "Start worker thread" << std::endl;
    zmq::socket_t socket(zmq_context, ZMQ_REP);
    socket.connect("inproc://workers");

    zmq::socket_t monitor(zmq_context, ZMQ_PUSH);
    monitor.connect("tcp://localhost:5558");

    while (true) {
      // Batch
      std::vector<inf_request> req_batch;

      while (true) {
        // Get request
        zmq::message_t req;
        int more;
        socket.recv(req);
        size_t more_size = sizeof(more);
        socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        //std::cout << "worker: recv, more = " << more << std::endl;
  
        // Deserialize
        inf_request req_obj;
        req_obj.unpack(static_cast<const char*>(req.data()), req.size());
        req_batch.push_back(req_obj);

        if (!more) break;
      }

      // Run
      stop_watch sw;
      std::vector<inf_reply> rep_batch;
      model->infer(req_batch, rep_batch);
      std::cout << "Worker " << worker_id << " : inf, bs=" << req_batch.size() <<
        ", latency=" << (sw.get_time_ns()/1e6) << std::endl;

      inf_event ev;
      ev.float_params[model->name + "_latency"] = float(sw.get_time_ns())/1e6;
      ev.send(monitor);

      for (auto i=0u; i<rep_batch.size(); i++) {
        //std::cout << "worker: send, " << i << std::endl;
        // Serialize
        std::stringstream ss;
        msgpack::pack(ss, rep_batch[i]);

        // Send reply
        zmq::message_t rep(ss.str());
        socket.send(rep, i+1 < rep_batch.size() ? zmq::send_flags::sndmore : zmq::send_flags::none);
      }
    }
  }

};

}
