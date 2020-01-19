#include "oneflow/core/kernel/kernel.h"
#include "oneflow/core/kernel/new_kernel_util.h"

namespace oneflow {

template<DeviceType device_type, typename T>
class ConstantLikeKernel final : public KernelIf<device_type> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(ConstantLikeKernel);
  ConstantLikeKernel() : is_init_(false) {}
  ~ConstantLikeKernel() = default;

 private:
  mutable bool is_init_;

  void ForwardDataContent(const KernelCtx& ctx,
                          std::function<Blob*(const std::string&)> BnInOp2Blob) const override {
    if (is_init_) { return; }
    Blob* out_blob = BnInOp2Blob("out");
    T value = 0;
    const auto& conf = this->op_conf().constant_like_conf();
    if (conf.has_int_operand()) {
      value = static_cast<T>(conf.int_operand());
    } else if (conf.has_float_operand()) {
      value = static_cast<T>(conf.float_operand());
    } else {
      UNIMPLEMENTED();
    }
    NewKernelUtil<device_type>::Fill(ctx.device_ctx, out_blob->static_shape().elem_cnt(), value,
                                     out_blob->mut_dptr<T>());
    is_init_ = true;
  }
};

#define REGISTER_CONSTANT_LIKE_KERNEL(dtype)                                                      \
  REGISTER_KERNEL_WITH_DEVICE_AND_DTYPE(OperatorConf::kConstantLikeConf, DeviceType::kCPU, dtype, \
                                        ConstantLikeKernel<DeviceType::kCPU, dtype>)              \
  REGISTER_KERNEL_WITH_DEVICE_AND_DTYPE(OperatorConf::kConstantLikeConf, DeviceType::kGPU, dtype, \
                                        ConstantLikeKernel<DeviceType::kGPU, dtype>)

REGISTER_CONSTANT_LIKE_KERNEL(float);
REGISTER_CONSTANT_LIKE_KERNEL(double);
REGISTER_CONSTANT_LIKE_KERNEL(int8_t);
REGISTER_CONSTANT_LIKE_KERNEL(int32_t);
REGISTER_CONSTANT_LIKE_KERNEL(int64_t);

#undef REGISTER_CONSTANT_LIKE_KERNEL

}  // namespace oneflow
