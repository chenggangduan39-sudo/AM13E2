import numpy as np

elem_type_map = {
    1: ('f32', np.float32),
    2: ('u8', np.uint8),
    3: ('i8', np.int8),
    4: ('u16', np.uint16),
    5: ('i16', np.int16),
    6: ('i32', np.int32),
    7: ('i64', np.int64),
    8: ('string'),
    9: ('bool', np.bool),
    10: ('f16', np.float16),
    11: ('double', np.double),
    12: ('u32', np.uint32),
    13: ('u64', np.uint64),
    14: ('cpx64', np.complex64),
    15: ('cpx128', np.complex128),
    16: ('bf16'),
}

elem_size_map = {
    'f32': 4,
    'u8': 1,
    'i8': 1,
    'u16': 2,
    'i16': 2,
    'i32': 4,
    'i64': 8,
    'double': 8,
    'u32': 4,
    'u64': 8,
    'cpx64': 8,
    'bool': 8,
    'cpx128': 16
}

elem_type2onnx = {v[0]: k for k, v in elem_type_map.items()}

elem_type_transform_map = {
    'i64': 'i32',
    'u64': 'u32',
}

elem_type2np = {x[0]: x[1] for x in elem_type_map.values() if len(x) == 2}
