#pragma once
#include <map>
#include <string>
#include <vector>

#include "/workspace/VART/samples/common/common.h"

namespace demo {
  
struct inf_buffer {
  std::map<std::string, std::vector<float>> buffers;

  std::vector<float*> input_raw_buffers, output_raw_buffers;
  std::vector<std::unique_ptr<xir::Tensor>> input_tensors, output_tensors;
  std::vector<std::unique_ptr<vart::TensorBuffer>> inputs, outputs;
  std::vector<vart::TensorBuffer*> input_ptrs, output_ptrs;
  std::vector<std::vector<int32_t>> input_shapes;
  std::vector<std::vector<int32_t>> output_shapes;

  inf_buffer(vart::Runner* runner)
  {
    {
      int index = 0;
      for (const auto& t: runner->get_input_tensors()) {
        const auto& name = "input" + std::to_string(index);
        const auto& layer_name = t->get_name();
        const auto& dims = t->get_dims();
        const auto& size = t->get_element_num();
        input_shapes.push_back(dims);
        buffers.emplace(std::make_pair(name, std::vector<float>(size)));
        input_raw_buffers.push_back(buffers[name].data());
        input_tensors.push_back(xir::Tensor::create(layer_name, dims, xir::DataType::FLOAT, sizeof(float) * 8u));
        inputs.push_back(std::make_unique<CpuFlatTensorBuffer>(input_raw_buffers[index], input_tensors[index].get()));
        input_ptrs.push_back(inputs[index].get());
        index++;
      }
    }

    {
      int index = 0;
      for (const auto& t: runner->get_output_tensors()) {
        const auto& name = "output" + std::to_string(index);
        const auto& layer_name = t->get_name();
        const auto& dims = t->get_dims();
        const auto& size = t->get_element_num();
        output_shapes.push_back(dims);
        buffers.emplace(std::make_pair(name, std::vector<float>(size)));
        output_raw_buffers.push_back(buffers[name].data());
        output_tensors.push_back(xir::Tensor::create(layer_name, dims, xir::DataType::FLOAT, sizeof(float) * 8u));
        outputs.push_back(std::make_unique<CpuFlatTensorBuffer>(output_raw_buffers[index], output_tensors[index].get()));
        output_ptrs.push_back(outputs[index].get());
        index++;
      }
    }
  }
};

}

