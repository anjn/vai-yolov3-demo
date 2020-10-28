#include "msgpack.hpp"
#include "arg/arg.h"
#include "yaml-cpp/yaml.h"

#include "server/inf_server.hpp"

std::vector<demo::inf_model_config> load_model_configs(const YAML::Node& node)
{
  std::vector<demo::inf_model_config> confs;

  for (const auto& model: node)
  {
    mls::inf_model_config conf;
    const auto& params = model.second;
    conf.name = model.first.as<std::string>();
    conf.xmodel = params["xmodel"].as<std::string>();
    
    conf.num_runners = params["num_runners"].as<int>();
    conf.num_streams = params["num_streams"].as<int>();

    for (const auto& input: params["inputs"])
    {
      std::vector<int> shape;
      for (const auto& s: input.second["shape"])
      {
        shape.push_back(s.as<int>());
      }
      conf.inputs[input.first.as<std::string>()] = {
        input.second["layer"].as<std::string>(),
        shape
      };
    }

    for (const auto& output: params["outputs"])
    {
      std::vector<int> shape;
      for (const auto& s: output.second["shape"])
      {
        shape.push_back(s.as<int>());
      }
      conf.outputs[output.first.as<std::string>()] = {
        output.second["layer"].as<std::string>(),
        shape
      };
    }

    confs.push_back(conf);
  }

  return confs;
}

int main(int argc, char** argv)
{
  // Parse arguments
  arg_begin("", 0, 0);
  arg_s(config_file, "server_config.yml", "Config file");
  arg_end;

  YAML::Node node = YAML::LoadFile("server_config.yml");

  demo::inf_server_config server_conf;
  server_conf.address     = node["address"].as<std::string>();
  server_conf.num_threads = node["num_threads"].as<int>();
  server_conf.model_confs = load_model_configs(node["models"]);

  demo::inf_server server(server_conf);
  server.start();
}

