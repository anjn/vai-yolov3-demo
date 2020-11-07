#include <boost/lockfree/queue.hpp>

#include "msgpack.hpp"
#include "arg/arg.h"
#include "yaml-cpp/yaml.h"

#include "server/inf_server.hpp"

std::vector<demo::inf_model_config> load_model_configs(const YAML::Node& node)
{
  std::vector<demo::inf_model_config> confs;

  for (const auto& model: node)
  {
    demo::inf_model_config conf;
    const auto& params = model.second;
    conf.name = model.first.as<std::string>();
    conf.xmodel = params["xmodel"].as<std::string>();
    conf.num_workers = params["num_workers"].as<int>();
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
  server_conf.model_confs = load_model_configs(node["models"]);

  demo::inf_server server(server_conf);
  server.start();
}

