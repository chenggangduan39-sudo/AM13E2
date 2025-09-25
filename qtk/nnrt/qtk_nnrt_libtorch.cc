#include "ATen/core/ivalue_inl.h"
#include "c10/core/TensorImpl.h"
#include <torch/script.h>
#include <torch/torch.h>

using namespace torch;

extern "C" {
#include "qtk/nnrt/qtk_nnrt_libtorch.h"
}

class RTImpl {
  public:
    std::vector<jit::IValue> inputs_;
    c10::ivalue::TupleElements outputs_;
    jit::script::Module model_;
    RTImpl(const char *mdl_fn) { model_ = jit::load(mdl_fn); }
    int forward() {
        outputs_ = model_.forward(inputs_).toTuple()->elements();
        return 0;
    }
};

class ValWrapper {
  public:
    at::Tensor tensor_;
};

extern "C" {
qtk_nnrt_libtorch_t *qtk_nnrt_libtorch_new(qtk_nnrt_cfg_t *cfg) {
    RTImpl *impl;
    qtk_nnrt_libtorch_t *rt =
        (qtk_nnrt_libtorch_t *)wtk_malloc(sizeof(qtk_nnrt_libtorch_t));
    rt->cfg = cfg;
    rt->impl = impl = new RTImpl((const char *)cfg->model);
    return rt;
}

void qtk_nnrt_libtorch_delete(qtk_nnrt_libtorch_t *rt) {
    RTImpl *impl = (class RTImpl *)rt->impl;
    delete (impl);
    wtk_free(rt);
}

int qtk_nnrt_libtorch_run(qtk_nnrt_libtorch_t *rt) {
    RTImpl *impl = (class RTImpl *)rt->impl;
    impl->forward();
    return 0;
}

int qtk_nnrt_libtorch_reset(qtk_nnrt_libtorch_t *rt) { return 0; }

int qtk_nnrt_libtorch_feed(qtk_nnrt_libtorch_t *rt, qtk_nnrt_value_t value,
                           int idx) {
    RTImpl *impl = (class RTImpl *)rt->impl;
    ValWrapper *val = (ValWrapper *)value;
    if (idx >= impl->inputs_.size()) {
        impl->inputs_.resize(idx + 1);
    }
    impl->inputs_[idx] = val->tensor_;
    return 0;
}

int qtk_nnrt_libtorch_get_output(qtk_nnrt_libtorch_t *rt, qtk_nnrt_value_t *out,
                                 int idx) {
    RTImpl *impl = (class RTImpl *)rt->impl;
    auto value = new ValWrapper();
    value->tensor_ = impl->outputs_[idx].toTensor();
    *out = value;
    return 0;
}

qtk_nnrt_value_t qtk_nnrt_libtorch_value_create_external(
    qtk_nnrt_libtorch_t *rt, qtk_nnrt_value_elem_type_t t, int64_t *shape,
    int shape_len, void *data) {
    auto value = new ValWrapper();
    auto options = TensorOptions().dtype(torch::kFloat32);
    if (t != QTK_NNRT_VALUE_ELEM_F32) {
        return NULL;
    }
    value->tensor_ = torch::from_blob(
        data, c10::ArrayRef<int64_t>(shape, shape_len), options);
    return value;
}

void qtk_nnrt_libtorch_value_release(qtk_nnrt_libtorch_t *rt,
                                     qtk_nnrt_value_t value) {
    ValWrapper *val = (ValWrapper *)value;
    delete (val);
}

int qtk_nnrt_libtorch_value_get_shape(qtk_nnrt_libtorch_t *rt,
                                      qtk_nnrt_value_t value, int64_t *shape,
                                      int shape_cap) {
    ValWrapper *val = (ValWrapper *)value;
    auto sizes = val->tensor_.sizes();
    int shape_dim = sizes.size();
    if (shape_dim > shape_cap) {
        return -1;
    }
    memcpy(shape, sizes.data(), sizeof(int64_t) * shape_dim);
    return shape_dim;
}

void *qtk_nnrt_libtorch_value_get_data(qtk_nnrt_libtorch_t *rt,
                                       qtk_nnrt_value_t value) {
    ValWrapper *val = (ValWrapper *)value;
    return val->tensor_.data_ptr();
}
}
