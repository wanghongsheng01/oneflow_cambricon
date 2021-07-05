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

#include "oneflow/core/framework/random_generator.h"

namespace oneflow {
namespace one {

Maybe<void> ManualSeed(uint64_t seed) {
  JUST(GetDefaultAutoGenerator())->set_current_seed(seed);
  return Maybe<void>::Ok();
}

namespace detail {

uint64_t GetNonDeterministicRandom() {
  std::random_device rd;
  // limit to 53 bits to ensure unique representation in double
  auto s = ((((uint64_t)rd()) << 32) + rd()) & 0x1FFFFFFFFFFFFF;
  return s;
}

}  // namespace detail

Generator::Generator(const std::shared_ptr<AutoGeneratorImpl>& impl) : impl_(impl) {}

Generator::Generator(const std::shared_ptr<GeneratorImpl>& impl) {
  impl_ = std::make_shared<AutoGeneratorImpl>(impl);
}

uint64_t Generator::current_seed() const { return impl_->current_seed(); }

void Generator::set_current_seed(uint64_t seed) { impl_->set_current_seed(seed); }

uint64_t Generator::seed() {
  uint64_t seed = detail::GetNonDeterministicRandom();
  set_current_seed(seed);
  return seed;
}

Maybe<Generator> GetDefaultAutoGenerator() {
  static auto default_auto_generator = std::make_shared<Generator>(
      std::make_shared<AutoGeneratorImpl>(detail::GetNonDeterministicRandom()));
  return default_auto_generator;
}

Maybe<Generator> GetDefaultCPUGenerator() {
  static auto default_cpu_generator =
      std::make_shared<Generator>(JUST(JUST(GetDefaultAutoGenerator())->Get<DeviceType::kCPU>()));
  return default_cpu_generator;
}

#ifdef WITH_CUDA
Maybe<Generator> GetDefaultCUDAGenerator() {
  static auto default_cuda_generator =
      std::make_shared<Generator>(JUST(JUST(GetDefaultAutoGenerator())->Get<DeviceType::kGPU>()));
  return default_cuda_generator;
}
#endif  // WITH_CUDA

template<>
Maybe<Generator> GetDefaultDeviceGenerator<DeviceType::kCPU>() {
  return GetDefaultCPUGenerator();
}

#ifdef WITH_CUDA
template<>
Maybe<Generator> GetDefaultDeviceGenerator<DeviceType::kGPU>() {
  return GetDefaultCUDAGenerator();
}
#endif  // WITH_CUDA

Maybe<Generator> MakeAutoGenerator(uint64_t seed) {
  return std::make_shared<Generator>(std::make_shared<AutoGeneratorImpl>(seed));
}

template<>
Maybe<Generator> MakeDeviceGenerator<DeviceType::kCPU>(uint64_t seed) {
  return std::make_shared<Generator>(std::make_shared<CPUGeneratorImpl>(seed));
}

#ifdef WITH_CUDA
template<>
Maybe<Generator> MakeDeviceGenerator<DeviceType::kGPU>(uint64_t seed) {
  return std::make_shared<Generator>(std::make_shared<CUDAGeneratorImpl>(seed));
}
#endif  // WITH_CUDA

Maybe<Generator> MakeGenerator(const std::string& device) {
  return MakeGenerator(device, detail::GetNonDeterministicRandom());
}
Maybe<Generator> MakeGenerator(const std::string& device, uint64_t seed) {
  if (device == "cpu") {
    return MakeDeviceGenerator<DeviceType::kCPU>(seed);
  }
#ifdef WITH_CUDA
  else if (device == "cuda" || device == "gpu") {
    return MakeDeviceGenerator<DeviceType::kGPU>(seed);
  }
#endif  // WITH_CUDA
  else if (device == "auto") {
    return MakeAutoGenerator(seed);
  } else {
    UNIMPLEMENTED_THEN_RETURN() << "Invalid device " << device
                                << " for making generator, please make sure the device is one of "
                                   "\"cpu\", \"gpu\" and \"auto\".";
  }
}

}  // namespace one
}  // namespace oneflow
