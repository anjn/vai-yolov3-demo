#pragma once
#include <map>
#include <string>
#include <vector>

#include "utils/queue_mt.hpp"
#include "server/inf_model_config.hpp"

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

struct inf_inout {
  std::map<std::string, std::vector<float>> buffers;

  std::vector<std::vector<float*>> input_batch;
  std::vector<float**> input_ptrs;
  std::vector<char*> input_names;

  std::vector<float*> output_ptrs;
  std::vector<unsigned> output_sizes;
  std::vector<char*> output_names;

  inf_inout(const inf_model_config& conf)
  {
    {
      int index = 0;
      for (auto& kv: conf.inputs) {
        auto& name = kv.first;
        auto& layer_name = kv.second.name;
        auto& shape = kv.second.shape;
        assert(shape.size() == 3);
        int size = shape[0] * shape[1] * shape[2];
        buffers.emplace(std::make_pair(name, std::vector<float>(size)));
        input_batch.emplace_back(1);
        input_batch[index][0] = buffers[name].data();
        input_ptrs.push_back(input_batch[index].data());
        input_names.push_back((char*)layer_name.c_str());
        index++;
      }
    }

    {
      int index = 0;
      for (auto& kv: conf.outputs) {
        auto& name = kv.first;
        auto& layer_name = kv.second.name;
        auto& shape = kv.second.shape;
        assert(shape.size() == 3);
        int size = shape[0] * shape[1] * shape[2];
        buffers.emplace(std::make_pair(name, std::vector<float>(size)));
        output_ptrs.push_back(buffers[name].data());
        output_sizes.push_back(size);
        output_names.push_back((char*)layer_name.c_str());
        index++;
      }
    }
  }
};

class inf_runner
{
  bool initialized { false };
  std::vector<inf_inout> inouts;
  queue_mt<int> stream_ids;

public:
  void init(const inf_model_config& conf)
  {
    // Create buffers and streams
    for (int i=0; i<conf.num_streams; i++) {
      inouts.emplace_back(conf);
      stream_ids.push(i);
    }
  
    // Create runner
    assert_file_exists(conf.xmodel);
    // TODO
  }

  int pop_stream_id() {
    return stream_ids.pop();
  }

  void push_stream_id(int stream_id) {
    stream_ids.push(stream_id);
  }
  
  inf_inout& get_buffer(int stream_id) {
    return inouts[stream_id];
  }

  void execute(int stream_id) {
    auto& inout = get_buffer(stream_id);
    auto batch_size = 1;
    //XDNNExecute_2D_float(executor,
    //  (const float***)inout.input_ptrs.data(),
    //  inout.input_names.data(),
    //  inout.input_ptrs.size(),
    //  inout.output_ptrs.data(),
    //  inout.output_sizes.data(),
    //  inout.output_names.data(),
    //  inout.output_ptrs.size(),
    //  batch_size, stream_id, true);
  }

  void stop() {
    stream_ids.stop();
  }
};

}
