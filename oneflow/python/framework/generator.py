"""
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
"""
from __future__ import absolute_import
import oneflow
import oneflow._oneflow_internal
from oneflow.python.oneflow_export import oneflow_export


@oneflow_export("Generator")
def MakeGenerator(device=None, seed=None):
    if device is None:
        device = "auto"
    if seed is None:
        return oneflow._oneflow_internal.create_generator(device)
    else:
        return oneflow._oneflow_internal.create_generator(device, seed)


@oneflow_export("manual_seed")
def manual_seed(seed):
    oneflow._oneflow_internal.manual_seed(seed)
