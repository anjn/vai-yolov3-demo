#pragma once

#include "server/inf_message.hpp"
#include "server/inf_model_config.hpp"
#include "server/inf_runner.hpp"

namespace demo {

struct inf_model_base
{
  const inf_model_config& conf;
  const std::string& name;
  inf_runner runner;

  inf_model_base(const inf_model_config& conf)
  : conf(conf), name(conf.name)
  {
    runner.init(conf);
  }

  virtual void infer(inf_request& req, inf_reply& rep) = 0;
};

}
