#ifdef USE_CUDNN
#include <algorithm>
#include <cfloat>
#include <vector>

#include "thrust/device_vector.h"

#include "caffe/layer.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype>
void CuDNNSoftmaxLayer<Dtype>::Forward_gpu(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  const Dtype* bottom_data = bottom[0]->gpu_data();
  Dtype* top_data = (*top)[0]->mutable_gpu_data();

  Dtype alpha = 1.0;
  Dtype beta = 0.0;

  CUDNN_CHECK(cudnnSoftmaxForward(handle_, CUDNN_SOFTMAX_ACCURATE,
      CUDNN_SOFTMAX_MODE_CHANNEL,
      reinterpret_cast<void *>(&alpha),
      bottom_desc_, bottom_data,
      reinterpret_cast<void *>(&beta),
      top_desc_, top_data));
}

template <typename Dtype>
void CuDNNSoftmaxLayer<Dtype>::Backward_gpu(const vector<Blob<Dtype>*>& top,
    const vector<bool>& propagate_down, vector<Blob<Dtype>*>* bottom) {
  if (propagate_down[0]) {
    const Dtype* top_data = top[0]->gpu_data();
    const Dtype* top_diff = top[0]->gpu_diff();
    const Dtype* bottom_data = (*bottom)[0]->gpu_data();
    Dtype* bottom_diff = (*bottom)[0]->mutable_gpu_diff();

    Dtype alpha = 1.0;
    Dtype beta = 0.0;

    CUDNN_CHECK(cudnnSoftmaxBackward(handle_, CUDNN_SOFTMAX_ACCURATE,
        CUDNN_SOFTMAX_MODE_CHANNEL,
        reinterpret_cast<void *>(&alpha),
        top_desc_, top_data, top_desc_, top_diff,
        reinterpret_cast<void *>(&beta),
        bottom_desc_, bottom_diff));
  }
}

INSTANTIATE_CLASS(CuDNNSoftmaxLayer);

}  // namespace caffe
#endif
