#pragma once
#include <map>
#include <string>
#include <vector>

namespace demo {
  
struct inf_model_config
{
  std::string name;
  std::string xmodel;

  struct layer {
    std::string name; // layer name
    std::vector<int> shape; // WHC
  };
  std::map<std::string, layer> inputs; // name -> layer
  std::map<std::string, layer> outputs; // name -> layer

  int num_runners;
  int num_streams;
};

}
