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
#ifndef ONEFLOW_CORE_THREAD_CAMBRICON_DEVICE_THREAD_H_
#define ONEFLOW_CORE_THREAD_CAMBRICON_DEVICE_THREAD_H_

#ifdef WITH_CAMBRICON
#include "oneflow/core/thread/thread.h"

namespace oneflow {

class CambriconDeviceThread final : public Thread {
 public:
  OF_DISALLOW_COPY_AND_MOVE(CambriconDeviceThread);
  CambriconDeviceThread() = delete;
  ~CambriconDeviceThread();

  CambriconDeviceThread(int64_t thrd_id, int64_t dev_id);

 private:
  Channel<CambriconCBNotifier> cb_notifier_chan_;
  std::thread cb_notifier_poller_;
};

}  // namespace oneflow

#endif  // WITH_CAMBRICON
#endif  // ONEFLOW_CORE_THREAD_CAMBRICON_DEVICE_THREAD_H_
