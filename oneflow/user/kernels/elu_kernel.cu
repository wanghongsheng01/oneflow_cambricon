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
#ifdef WITH_CUDA
#include "oneflow/core/kernel/util/cuda_half_util.h"
#include "oneflow/core/cuda/elementwise.cuh"
#include "oneflow/user/kernels/elu_kernel.h"
namespace oneflow {

template<>
struct EluFunctor<half> {
  OF_DEVICE_FUNC explicit EluFunctor(float alpha)
      : alpha(alpha), float_functor(EluFunctor<float>(alpha)) {}
  OF_DEVICE_FUNC half operator()(half x) const {
    return __float2half(float_functor(__half2float(x)));
  }
  const float alpha;
  EluFunctor<float> float_functor;
};

template<>
struct EluGradFunctor<half> {
  OF_DEVICE_FUNC explicit EluGradFunctor(float alpha)
      : alpha(alpha), float_functor(EluGradFunctor<float>(alpha)) {}
  OF_DEVICE_FUNC half operator()(half x, half dy) const {
    return __float2half(float_functor(__half2float(x), __half2float(dy)));
  }
  const float alpha;
  EluGradFunctor<float> float_functor;
};

namespace {

template<template<typename> class Opt, typename T>
struct ElemwiseEluFunctor<DeviceType::kGPU, Opt, T> {
  void operator()(DeviceCtx* ctx, const int64_t elem_cnt, T alpha, T* out, const T* in) {
    OF_CUDA_CHECK(oneflow::cuda::elementwise::Unary(EluFunctor<T>(alpha), elem_cnt, out, in,
                                                    ctx->cuda_stream()));
  }
};

template<template<typename> class Opt, typename T>
struct ElemwiseEluGradFunctor<DeviceType::kGPU, Opt, T> {
  void operator()(DeviceCtx* ctx, const int64_t elem_cnt, T alpha, T* dx, const T* x, const T* dy) {
    OF_CUDA_CHECK(oneflow::cuda::elementwise::Binary(EluGradFunctor<T>(alpha), elem_cnt, dx, x, dy,
                                                     ctx->cuda_stream()));
  }
};

}  // namespace

REGISTER_ELU_KERNELS(DeviceType::kGPU, half);
REGISTER_ELU_KERNELS(DeviceType::kGPU, float);
REGISTER_ELU_KERNELS(DeviceType::kGPU, double);

}  // namespace oneflow
#endif
