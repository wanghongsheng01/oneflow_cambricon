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
class Generator(object):
    def __init__(self):
        self._gen = oneflow._oneflow_internal.one.Generator()

    def manual_seed(self, seed):
        self._gen.manual_seed(seed)

    @property
    def gen(self):
        return self._gen