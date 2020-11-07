#pragma once

#include "server/inf_buffer.hpp"
#include "server/inf_message.hpp"
#include "server/inf_model_config.hpp"

namespace demo {

struct inf_model_base
{
  const inf_model_config& conf;
  const std::string& name;

  inf_buffer buf;
  std::unique_ptr<vart::Runner> dpu_runner;

  inf_model_base(const inf_model_config& conf, const xir::Subgraph* subgraph)
  : conf(conf), name(conf.name)
  {
    dpu_runner = vart::Runner::create_runner(subgraph, "run");
    buf = inf_buffer(dpu_runner.get());
  }

  virtual void infer(inf_request& req, inf_reply& rep) = 0;

  void execute() {
    auto job_id = dpu_runner->execute_async(buf.input_ptrs, buf.output_ptrs);
    dpu_runner->wait(job_id.first, -1);
  }
};

}
