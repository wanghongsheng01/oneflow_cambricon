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
import oneflow.core.register.logical_blob_id_pb2 as logical_blob_id_util
import oneflow.python.framework.c_api_util as c_api_util
import oneflow.python.framework.id_util as id_util
import oneflow.python.framework.placement_context as placement_ctx
import oneflow.python.framework.blob_trait as blob_trait
from oneflow.python.framework.dtype import convert_proto_dtype_to_oneflow_dtype
import oneflow.python.lib.core.enable_if as enable_if
import oneflow.python.framework.hob as hob
import oneflow.python.eager.eager_blob_util as eager_blob_util
import oneflow.python.eager.blob_cache as blob_cache_util
import oneflow.python.eager.gradient_util as gradient_util
import oneflow.python.eager.boxing_util as boxing_util
import oneflow_api.oneflow.core.job.placement as placement_cfg
import oneflow_api.oneflow.core.register.logical_blob_id as lbi_util
import oneflow_api
import traceback
import sys

blob_register = oneflow_api.GetDefaultBlobRegister()


def RemoteBlob(lbi, **kw):
    api = enable_if.unique([EagerLogicalBlob, LazyRemoteBlob])
    return api(lbi, **kw)


@enable_if.condition(hob.in_global_mode & hob.eager_execution_enabled)
def EagerLogicalBlob(lbi, **kw):
    job_name = oneflow_api.JobBuildAndInferCtx_GetCurrentJobName()
    lbn = lbi.op_name + "/" + lbi.blob_name
    if not isinstance(lbi, lbi_util.LogicalBlobId):
        cfg_lbi = lbi_util.LogicalBlobId()
        cfg_lbi.set_op_name(lbi.op_name)
        cfg_lbi.set_blob_name(lbi.blob_name)
        lbi = cfg_lbi
    blob_type = oneflow_api.EagerConsistentBlob
    if c_api_util.JobBuildAndInferCtx_IsMirroredBlob(job_name, lbn):
        blob_type = oneflow_api.EagerMirroredBlob
    job_name = ""
    if ("job_name" in kw) and (kw["job_name"] is not None):
        job_name = kw["job_name"]
    blob_object = None
    if "blob_object" in kw:
        blob_object = kw["blob_object"]
    distribute = oneflow_api.distribute.auto()
    if "distribute" in kw:
        distribute = kw["distribute"]
    return blob_type(lbi, blob_object, blob_register, job_name, distribute)


@enable_if.condition(~hob.eager_execution_enabled)
def LazyRemoteBlob(lbi, **kw):
    job_name = oneflow_api.JobBuildAndInferCtx_GetCurrentJobName()
    lbn = lbi.op_name + "/" + lbi.blob_name
    blob_type = oneflow_api.LazyConsistentBlob
    if c_api_util.JobBuildAndInferCtx_IsMirroredBlob(job_name, lbn):
        blob_type = oneflow_api.LazyMirroredBlob
    if not isinstance(lbi, lbi_util.LogicalBlobId):
        cfg_lbi = lbi_util.LogicalBlobId()
        cfg_lbi.set_op_name(lbi.op_name)
        cfg_lbi.set_blob_name(lbi.blob_name)
        lbi = cfg_lbi
    job_name = ""
    if ("job_name" in kw) and (kw["job_name"] is not None):
        job_name = kw["job_name"]
    distribute = oneflow_api.distribute.auto()
    if "distribute" in kw:
        distribute = kw["distribute"]
    return blob_type(lbi, job_name, distribute)


@property
def dtype(self):
    ret = convert_proto_dtype_to_oneflow_dtype(self.get_dtype())
    assert isinstance(ret, oneflow.dtype)
    return ret


def with_distribute(self, distribute):
    new = type(self)(self.lbi, self.job_name, oneflow_api.distribute.auto())
    new.set_distribute(distribute)
    return new


def with_gradient_distribute(self, distribute):
    return oneflow.parallel_cast(self, gradient_distribute=distribute)


def get_lazy_shape_log_warning(self):
    if oneflow.scope.mirrored_view_enabled():
        return ("%s\n%s\n%s") % (
            "WARNING:",
            "You access a consistent blob shape in mirrored view, there may be problems,",
            "you should add 'x = flow.cast_to_current_logical_view(x)'.",
        )
    else:
        return ""


def get_mirror_shape_log_warning(self):
    if oneflow.scope.consistent_view_enabled():
        return ("%s\n%s\n%s") % (
            "WARNING:",
            "You access a mirrored blob shape in consistent view, there may be problems,",
            "you should add 'x = flow.cast_to_current_logical_view(x)'.",
        )
    else:
        return ""


def RegisterMethod4BlobDef(blob_class):
    blob_class.dtype = dtype
    blob_class.with_distribute = with_distribute
    blob_class.with_gradient_distribute = with_gradient_distribute


def RegisterMethod4LazyConsistentBlob():
    RegisterMethod4BlobDef(oneflow_api.LazyConsistentBlob)
    oneflow_api.LazyConsistentBlob.get_lazy_shape_log_warning = (
        get_lazy_shape_log_warning
    )


def RegisterMethod4LazyMirroredBlob():
    RegisterMethod4BlobDef(oneflow_api.LazyMirroredBlob)
    oneflow_api.LazyMirroredBlob.get_mirror_shape_log_warning = (
        get_mirror_shape_log_warning
    )


@property
def sub_consistent_blob_list(self):
    raise NotImplementedError


def numpy(self, rank=None):
    assert rank is None or rank == 0
    return self._Numpy()


def numpy_list(self, rank=None):
    assert rank is None or rank == 0
    return [self._Numpy()]


def BlobObjectNumpy(blob_object, parallel_conf, tmp_name=None):
    if tmp_name is None:
        tmp_name = id_util.UniqueStr("numpy-tmp-")

    def FetchBlobNumpy(blob_object):
        consistent_blob_name = None

        def BoxingToSingleDevice(builder):
            parallel_conf = placement_cfg.ParallelConf()
            parallel_conf.set_device_tag(blob_object.parallel_desc_symbol.device_tag)
            parallel_conf.add_device_name("{}:{}".format(0, 0))
            tmp_parallel_desc_symbol = builder.GetParallelDescSymbol(parallel_conf)
            tmp_op_arg_parallel_attr = oneflow_api.OpArgParallelAttribute(
                tmp_parallel_desc_symbol,
                str(blob_object.op_arg_parallel_attr.sbp_parallel),
                str(blob_object.op_arg_parallel_attr.opt_mirrored_parallel),
            )
            with oneflow.scope.placement(
                parallel_conf.device_tag(), list(parallel_conf.device_name()),
            ):
                tmp_blob_object = boxing_util.BoxingTo(
                    builder, blob_object, tmp_op_arg_parallel_attr
                )
            nonlocal consistent_blob_name
            consistent_blob_name = tmp_name
            if not blob_register.HasObject4BlobName(consistent_blob_name):
                blob_register.SetObject4BlobName(consistent_blob_name, tmp_blob_object)

        oneflow_api.deprecated.LogicalRun(BoxingToSingleDevice)
        return oneflow_api.EagerPhysicalBlob(
            consistent_blob_name,
            blob_register,
            eager_blob_util._GetPhysicalBlobHeaderCache,
        ).numpy()

    blob_cache = oneflow_api.FindOrCreateBlobCache(blob_object)
    return blob_cache.GetCachedNumpy(FetchBlobNumpy)


def _Numpy(self):
    tmp_name = "{}-consistent".format(self.logical_blob_name)
    return BlobObjectNumpy(self.blob_object, self.parallel_conf, tmp_name)


def RegisterMethod4EagerBlobTrait():
    oneflow_api.EagerBlobTrait.sub_consistent_blob_list = sub_consistent_blob_list
    oneflow_api.EagerBlobTrait.dtype = dtype
    oneflow_api.EagerBlobTrait._Numpy = _Numpy
    oneflow_api.EagerBlobTrait.numpy = numpy
    oneflow_api.EagerBlobTrait.numpy_list = numpy_list


def eager_with_distribute(self, distribute):
    new = type(self)(
        self.lbi,
        blob_object=self.blob_object,
        blob_register=blob_register,
        job_name=self.job_name,
        distribute=self.distribute,
    )
    new.set_distribute(distribute)
    return new


def RegisterMethod4EagerConsistentBlob():
    oneflow_api.EagerConsistentBlob.dtype = dtype
    oneflow_api.EagerConsistentBlob.with_distribute = eager_with_distribute
    oneflow_api.EagerConsistentBlob.with_gradient_distribute = with_gradient_distribute
