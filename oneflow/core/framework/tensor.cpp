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
#include "oneflow/core/framework/tensor.h"
#include "oneflow/core/job/parallel_desc.h"
#include "oneflow/core/framework/device.h"
#include "oneflow/core/framework/dtype.h"
#include "oneflow/core/autograd/autograd_engine.h"

namespace oneflow {

namespace one {

Maybe<int64_t> FacadeTensor::ndim() const {
  return JUST(tensor_->DetermineAndDestroySelf())->ndim();
}

Maybe<bool> FacadeTensor::is_cuda() const {
  return JUST(tensor_->DetermineAndDestroySelf())->is_cuda();
}

Maybe<int64_t> FacadeTensor::nelement() const {
  return JUST(tensor_->DetermineAndDestroySelf())->nelement();
}

Maybe<int64_t> FacadeTensor::dim(int64_t index) const {
  return JUST(tensor_->DetermineAndDestroySelf())->dim(index);
}

Maybe<const FunctionNode> FacadeTensor::grad_fn_node() const {
  return JUST(tensor_->DetermineAndDestroySelf())->grad_fn_node();
}

Maybe<Tensor> FacadeTensor::acc_grad() const {
  return JUST(tensor_->DetermineAndDestroySelf())->acc_grad();
}

Maybe<const compatible_py::Distribute> UndeterminedTensor::distribute() const {
  CHECK_OR_RETURN(distribute_) << Error::ValueError("Distribute is not determined.");
  return distribute_;
}

Maybe<const ParallelDesc> UndeterminedTensor::parallel_desc() const {
  CHECK_OR_RETURN(parallel_desc_) << Error::ValueError("Parallel_desc undetermined");
  return parallel_desc_;
}

Maybe<const Device> UndeterminedTensor::device() const {
  CHECK_OR_RETURN(device_) << Error::ValueError("Device undetermined.");
  return device_;
}

Maybe<DeterminedTensor> UndeterminedTensor::DetermineAndDestroySelf() {
  if (JUST(is_consistent())) {
    return std::static_pointer_cast<DeterminedTensor>(ConsistentTensor::MakeTensor(
        JUST(shape()), JUST(dtype()), JUST(distribute()), JUST(parallel_desc()), JUST(is_lazy()),
        JUST(requires_grad()), JUST(is_leaf()), JUST(retain_grad())));
  } else {
    return std::static_pointer_cast<DeterminedTensor>(
        MirroredTensor::MakeTensor(JUST(shape()), JUST(dtype()), JUST(device()), JUST(is_lazy()),
                                   JUST(requires_grad()), JUST(is_leaf()), JUST(retain_grad())));
  }
}

std::shared_ptr<MirroredTensor> MirroredTensor::MakeTensor(
    const std::shared_ptr<const Shape>& shape, const std::shared_ptr<const DType>& dtype,
    const std::shared_ptr<const Device>& device, bool is_lazy, bool requires_grad, bool is_leaf,
    bool retain_grad) {
  std::shared_ptr<MirroredTensorImpl> impl;
  if (is_lazy) {
    impl = std::make_shared<LazyMirroredTensorImpl>(shape, dtype, device, requires_grad, is_leaf,
                                                    retain_grad);
  } else {
    impl = std::make_shared<EagerMirroredTensorImpl>(shape, dtype, device, requires_grad, is_leaf,
                                                     retain_grad);
  }
  return std::make_shared<MirroredTensor>(impl);
}

Maybe<bool> MirroredTensor::is_cuda() const { return JUST(device())->type() == "cuda"; }

Maybe<int64_t> MirroredTensor::ndim() const { return JUST(shape())->NumAxes(); }

Maybe<int64_t> MirroredTensor::dim(int64_t index) const { return JUST(shape())->At(index); }

Maybe<int64_t> MirroredTensor::nelement() const { return JUST(shape())->elem_cnt(); }

std::shared_ptr<ConsistentTensor> ConsistentTensor::MakeTensor(
    const std::shared_ptr<const Shape>& shape, const std::shared_ptr<const DType>& dtype,
    const std::shared_ptr<const compatible_py::Distribute>& distribute,
    const std::shared_ptr<const ParallelDesc>& parallel_desc, bool is_lazy, bool requires_grad,
    bool is_leaf, bool retain_grad) {
  std::shared_ptr<ConsistentTensorImpl> impl;
  if (is_lazy) {
    impl = std::make_shared<LazyConsistentTensorImpl>(shape, dtype, distribute, parallel_desc,
                                                      requires_grad, is_leaf, retain_grad);
  } else {
    impl = std::make_shared<EagerConsistentTensorImpl>(shape, dtype, distribute, parallel_desc,
                                                       requires_grad, is_leaf, retain_grad);
  }
  return std::make_shared<ConsistentTensor>(impl);
}

Maybe<bool> ConsistentTensor::is_cuda() const {
  return JUST(parallel_desc())->device_type() == DeviceType::kGPU;
}

Maybe<int64_t> ConsistentTensor::dim(int64_t index) const { return JUST(shape())->At(index); }

Maybe<int64_t> ConsistentTensor::nelement() const { return JUST(shape())->elem_cnt(); }

Maybe<int64_t> ConsistentTensor::ndim() const { return JUST(shape())->NumAxes(); }

}  // namespace one

}  // namespace oneflow