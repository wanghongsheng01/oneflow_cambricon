/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef ONEFLOW_CORE_JOB_REWRITER_OP_GRAPH_PASS_H_
#define ONEFLOW_CORE_JOB_REWRITER_OP_GRAPH_PASS_H_

#include "oneflow/core/graph/op_graph.h"
#include "oneflow/core/job/job_builder.h"
#include "oneflow/core/common/util.h"

namespace oneflow {

class OpGraphPassState {
 public:
  OpGraphPassState(const OpGraphPassState&) = delete;
  OpGraphPassState(OpGraphPassState&&) = delete;
  OpGraphPassState() = default;
  ~OpGraphPassState() = default;
};

class OpGraphPass {
 public:
  explicit OpGraphPass(const JobDesc& job_desc) : job_desc_(&job_desc) {}
  virtual ~OpGraphPass() = default;

  Maybe<OpGraphPassState> operator()(const OpGraphPassState& state, Job* job) const {
    if (IsEnabled() == false) { return std::make_shared<OpGraphPassState>(); }
    return Apply(state, job);
  }
  virtual bool IsEnabled() const { return true; }
  virtual Maybe<OpGraphPassState> Apply(const OpGraphPassState& state, Job* job) const {
    OpGraph op_graph;
    JUST(op_graph.Init(*job));
    return Apply(state, op_graph, job);
  }
  virtual Maybe<OpGraphPassState> Apply(const OpGraphPassState& state, const OpGraph& op_graph,
                                        Job* job) const {
    JobBuilder job_builder(job);
    return Apply(state, op_graph, &job_builder);
  }
  virtual Maybe<OpGraphPassState> Apply(const OpGraphPassState& state, const OpGraph& op_graph,
                                        JobBuilder* job_builder) const {
    UNIMPLEMENTED();
    return std::make_shared<OpGraphPassState>();
  }

 protected:
  const JobDesc& job_desc() const { return *job_desc_; }

 private:
  const JobDesc* job_desc_;
};

#define REGISTER_FUNCTION_PASS(pass_name, pass_type) \
  COMMAND(RegisterFunctionPass(                      \
      pass_name, [](const JobDesc& job_desc) -> OpGraphPass* { return new pass_type(job_desc); }))

void RegisterFunctionPass(const std::string& pass_name,
                          const std::function<OpGraphPass*(const JobDesc&)>& pass_creator);
bool HasFunctionPass(const std::string& pass_name);
std::unique_ptr<const OpGraphPass> FunctionPass(const std::string& pass_name, const JobDesc&);

}  // namespace oneflow

#endif  // ONEFLOW_CORE_JOB_REWRITER_OP_GRAPH_PASS_H_
