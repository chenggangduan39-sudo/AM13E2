from typing import List, Tuple
import re
import sys
import numpy as np
import struct
import graph

class NCNNLayer:
    def __init__(self, name: str, bottoms: List[str], tops: List[str]):
        self.name = name
        self.bottoms = bottoms
        self.tops = tops

    def load_tensor(self, reader, shape, dtype):
        dtype = np.dtype(dtype)
        bytes_repl = reader.read(np.prod(shape) * dtype.itemsize)
        return np.frombuffer(bytes_repl, dtype=dtype).reshape(shape)

    def load_blob(self, reader, shape):
        tag_bytes = reader.read(4)
        tag = struct.unpack('<I', tag_bytes)[0]
        if tag == 0:
            return self.load_tensor(reader, shape, 'float32')
        raise Exception('unknown blob tag')

    def load_model(self, reader):
        pass

    def export(self):
        return [], []


class ParamDict:
    def __init__(self, raw_val):
        self.raw_val = raw_val

    def get(self, key, default):
        if key not in self.raw_val:
            return default
        val = self.raw_val[key]
        convert_type = type(default)
        return convert_type(val)


class NCNNConvolution(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        assert(10 not in pd.raw_val) # activation_params
        self.num_output = pd.get(1, 0)
        self.num_output = pd.get(0, 0)
        self.kernel_w = pd.get(1, 0)
        self.kernel_h = pd.get(11, self.kernel_w)
        self.dilation_w = pd.get(2, 1)
        self.dilation_h = pd.get(12, self.dilation_w)
        self.stride_w = pd.get(3, 1)
        self.stride_h = pd.get(13, self.stride_w)
        self.pad_left = pd.get(4, 0)
        self.pad_right = pd.get(15, self.pad_left)
        self.pad_top = pd.get(14, self.pad_left)
        self.pad_bottom = pd.get(16, self.pad_top)
        self.pad_value = pd.get(18, 0.0)
        self.bias_term = pd.get(5, False)
        self.weight_data_size = pd.get(6, 0)
        self.int8_scale_term = pd.get(8, 0)
        self.activation_type = pd.get(9, 0)
        self.dynamic_weight = pd.get(19, False)
        super().__init__(name, bottoms, tops)

    def load_model(self, reader):
        if self.dynamic_weight:
            return
        self.weight_data = self.load_blob(reader, [self.weight_data_size])
        if self.bias_term:
            self.bias_data = self.load_tensor(reader, [self.num_output], 'float32')

    def export(self) -> Tuple[List[graph.Node], List[graph.Tensor]]:
        weight_symbol = f'{self.name}_weight_data'
        bias_symbol = f'{self.name}_bias_data'
        inp = [self.bottoms[0], weight_symbol, bias_symbol]
        outp = [self.tops[0]]
        node = graph.Conv(self.name, inp, outp,
                          dilations = [self.dilation_h, self.dilation_w],
                          group = 1,
                          kernel_shape = [self.kernel_h, self.kernel_w],
                          strides = [self.stride_h, self.stride_w],
                          pads = [self.pad_top, self.pad_left, self.pad_bottom, self.pad_right])
        weight = graph.Tensor(self.weight_data, weight_symbol, 'f32')
        bias = graph.Tensor(self.bias_data, bias_symbol, 'f32') if self.bias_term else graph.Tensor(np.array([]), bias_symbol, 'empty')
        return [node], [weight, bias]


class NCNNErb(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.ibins = pd.get(0, 0);
        self.obins = pd.get(1, 0);
        self.raw_subband = pd.get(2, 0);
        super().__init__(name, bottoms, tops)

    def load_model(self, reader):
        self.weight_data = self.load_blob(reader, [self.obins, self.ibins])


class NCNNInnerProduct(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        assert(10 not in pd.raw_val) # activation_params
        self.num_output = pd.get(0, 0);
        self.bias_term = pd.get(1, False);
        self.weight_data_size = pd.get(2, 0);
        self.int8_scale_term = pd.get(8, 0);
        self.activation_type = pd.get(9, 0);
        super().__init__(name, bottoms, tops)

    def load_model(self, reader):
        self.weight_data = self.load_blob(reader, [self.weight_data_size])
        if self.bias_term:
            self.bias_data = self.load_tensor(reader, [self.num_output], 'float32')

    def export(self)-> Tuple[List[graph.Node], List[graph.Tensor]]:
        weight_symbol = f'{self.name}_weight_data'
        weight = graph.Tensor(self.weight_data, weight_symbol, 'f32')
        if self.bias_term:
            bias_symbol = f'{self.name}_bias_data'
            bias = graph.Tensor(self.bias_data, bias_symbol, 'f32')
            node = graph.Gemm(self.name, [self.bottoms[0], weight_symbol, bias_symbol], [self.tops[0]])
            return [node], [weight, bias]
        else:
            node = graph.MatMul(self.name, [self.bottoms[0], weight_symbol], [self.tops[0]])
            return [node], [weight]


class NCNNLSTM(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.num_output = pd.get(0, 0);
        self.weight_data_size = pd.get(1, 0);
        self.direction = pd.get(2, 0);
        self.hidden_size = pd.get(3, self.num_output);
        super().__init__(name, bottoms, tops)

    def load_model(self, reader):
        num_direction = 2 if self.direction == 2 else 1
        size = self.weight_data_size // num_direction // self.hidden_size // 4
        self.weight_xc_data = self.load_blob(reader, [num_direction, self.hidden_size * 4, size])
        self.bias_c_data = self.load_blob(reader, [num_direction, 4, self.hidden_size])
        self.weight_hc_data = self.load_blob(reader, [num_direction, self.hidden_size * 4, self.num_output])
        if (self.num_output != self.hidden_size):
            self.weight_hr_data = self.load_blob(reader, [num_direction, self.num_output, self.hidden_size])


class NCNNLayerNorm(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.affine_size = pd.get(0, 0);
        self.eps = pd.get(1, 0.001);
        self.affine = pd.get(2, 1);
        super().__init__(name, bottoms, tops)

    def load_model(self, reader):
        if self.affine_size == 0:
            return
        self.gamma_data = self.load_tensor(reader, [self.affine_size], 'float32')
        self.beta_data = self.load_tensor(reader, [self.affine_size], 'float32')


class NCNNLayerScale(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.channels = pd.get(0, 4);
        self.channel_dim = pd.get(1, 0);
        self.dims = pd.get(2, 2);
        self.mode = pd.get(3, 1);
        super().__init__(name, bottoms, tops)

    def load_model(self, reader):
        self.scale_data = self.load_tensor(reader, [self.channels], 'float32')


class NCNNPReLU(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.num_slope = pd.get(0, 0)
        super().__init__(name, bottoms, tops)

    def load_model(self, reader):
        self.slope_data = self.load_tensor(reader, [self.num_slope], 'float32')


class NCNNMultiChannelMask(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.ref_channel = pd.get(0, 0)
        self.mic_channels = pd.get(1, 1)
        self.mask_nonlinear = pd.get(2, 0) # 0: "linear", 1: "relu", 2: "sigmoid", 3: "tanh"
        self.mask_type = pd.get(3, 0) # 0: "crm", 1: "rm", 2: "crm2"
        self.use_mvdr = pd.get(4, True)
        self.substract_speech = pd.get(5, True)
        super().__init__(name, bottoms, tops)


class NCNNSplit(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        super().__init__(name, bottoms, tops)


class NCNNInput(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.w = pd.get(0, 0)
        self.h = pd.get(1, 0)
        self.d = pd.get(11, 0)
        self.c = pd.get(2, 0)
        super().__init__(name, bottoms, tops)


class NCNNConvToRnnFeat(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.batch_first = pd.get(0, False)
        self.freq_share = pd.get(1, False)
        self.inter = pd.get(2, False)
        super().__init__(name, bottoms, tops)


class NCNNRnnToConvFeat(NCNNLayer):
    def __init__(self, name, bottoms, tops, pd):
        self.nbins = pd.get(0, 33)
        self.nframes = pd.get(1, 1)
        self.batch_first = pd.get(2, False)
        self.freq_share = pd.get(3, False)
        self.inter = pd.get(4, False)
        super().__init__(name, bottoms, tops)


class NCNNGraph:
    def read_param(self, param_file):
        with open(param_file, 'r') as f:
            lines = [x.strip() for x in f]
        magic = int(lines[0])
        assert magic == 7767517
        layer_cnt, blob_cnt = [int(x) for x in lines[1].split()]
        layers = []
        graph_inputs = []
        current_module = sys.modules[__name__]
        for layer_index in range(layer_cnt):
            layer_type, layer_name, bottom_cnt, top_cnt, rest_params = lines[2 + layer_index].split(maxsplit=4)
            bottom_cnt, top_cnt = int(bottom_cnt), int(top_cnt)
            blobs = rest_params.split(maxsplit=(bottom_cnt + top_cnt))
            bottoms = blobs[:bottom_cnt]
            tops = blobs[bottom_cnt:bottom_cnt + top_cnt]
            items = {}
            if len(blobs) == bottom_cnt + top_cnt + 1:
                rest_params = blobs[bottom_cnt + top_cnt]
                items = [x.split('=') for x in rest_params.split()]
                items = {int(x[0]): x[1] for x in items}
            layer = getattr(current_module, f'NCNN{layer_type}')(layer_name, bottoms, tops, ParamDict(items))
            if isinstance(layer, NCNNInput):
                input_name = layer.tops[0]
                graph_inputs.append(input_name)
            else:
                layers.append(layer)
        self.graph_inputs = graph_inputs
        self.layers = layers


    def __init__(self, param_file, model_file, **kwargs):
        self.read_param(param_file)
        with open(model_file, 'rb') as f:
            [x.load_model(f) for x in self.layers]
        self.graph_outputs = kwargs['output_names']
        if 'shape_info_file' in  kwargs:
            with open(kwargs['shape_info_file'], 'rt') as f:
                infos = [x.split() for x in f]
            infos = {x[0]: x[1].split(',') for x in infos}
        self.shape_info = infos


if __name__ == "__main__":
    import sys
    nr = NCNNGraph(sys.argv[1], sys.argv[2], output_names = [f'out{x}' for x in range(5)], shape_info_file=sys.argv[3])
    [x.export() for x in nr.layers]
