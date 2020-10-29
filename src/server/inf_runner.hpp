#pragma once
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "server/inf_model_config.hpp"
#include "server/inf_buffer.hpp"
#include "utils/queue_mt.hpp"
#include "utils/time_util.hpp"

namespace demo {
  
static void assert_file_exists(const std::string& file)
{
  std::ifstream ifs(file);
  if (!ifs.is_open()) {
    std::cerr << "Error:  '" << file << "' doesn't exist!" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << file << std::endl;
}

class inf_runner
{
  std::vector<inf_buffer> buffers;
  queue_mt<int> stream_ids;
  std::unique_ptr<xir::Graph> graph;
  std::vector<const xir::Subgraph*> subgraph;
  std::unique_ptr<vart::Runner> dpu_runner;

public:
  void init(const inf_model_config& conf)
  {
    // Create runner
    assert_file_exists(conf.xmodel);

    graph = xir::Graph::deserialize(conf.xmodel);
    subgraph = get_dpu_subgraph(graph.get());
    dpu_runner = vart::Runner::create_runner(subgraph[0], "run");

    // Create buffers and streams
    for (int i=0; i<conf.num_streams; i++) {
      buffers.emplace_back(dpu_runner.get());
      stream_ids.push(i);
    }
  }

  int pop_stream_id() {
    return stream_ids.pop();
  }

  void push_stream_id(int stream_id) {
    stream_ids.push(stream_id);
  }
  
  auto& get_buffer(int stream_id) {
    return buffers[stream_id];
  }

  void execute(int stream_id) {
    auto& buf = get_buffer(stream_id);
    {
      auto& i = buf.buffers["input0"];
      //std::ofstream ofs("input0.raw", std::ios::binary);
      //ofs.write(reinterpret_cast<char*>(i.data()), sizeof(float) * i.size());
      //std::ifstream ifs("input0.raw", std::ios::binary);
      //ifs.read(reinterpret_cast<char*>(i.data()), sizeof(float) * i.size());
    }
    stop_watch sw;
    auto job_id = dpu_runner->execute_async(buf.input_ptrs, buf.output_ptrs);
    dpu_runner->wait(job_id.first, -1);
    {
      auto& i = buf.buffers["output0"];
      //std::ofstream ofs("output0.raw", std::ios::binary);
      //ofs.write(reinterpret_cast<char*>(i.data()), sizeof(float) * i.size());
      //std::ifstream ifs("output0.raw", std::ios::binary);
      //ifs.read(reinterpret_cast<char*>(i.data()), sizeof(float) * i.size());
    }
    //sw.time("dpu");
  }

  void stop() {
    stream_ids.stop();
  }
};

}
