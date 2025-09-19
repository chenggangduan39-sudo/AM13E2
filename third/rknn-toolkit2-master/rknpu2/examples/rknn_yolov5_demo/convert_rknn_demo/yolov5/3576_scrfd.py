
import cv2
import numpy as np

from rknn.api import RKNN
import os

if __name__ == '__main__':

    platform = 'rk3576'
    MODEL_PATH = 'scrfd_0.5_384x640_face.onnx'
    NEED_BUILD_MODEL = True

    # Create RKNN object
    rknn = RKNN()

    OUT_DIR = "rknn_models"
    RKNN_MODEL_PATH = './3576_scrfd_0.5_384x640_face.rknn'
    if NEED_BUILD_MODEL:
        DATASET = './dataset1.txt'
        rknn.config(mean_values=[[127.5, 127.5, 127.5]], std_values=[[128, 128, 128]], target_platform="rk3576")
        # Load model
        print('--> Loading model')
        ret = rknn.load_onnx(MODEL_PATH)
        if ret != 0:
            print('load model failed!')
            exit(ret)
        print('done')

        # Build model
        print('--> Building model')
        #ret = rknn.build(do_quantization=True, dataset=DATASET)
        ret = rknn.build(do_quantization=False, dataset=DATASET)
        if ret != 0:
            print('build model failed.')
            exit(ret)
        print('done')

        # Export rknn model
        if not os.path.exists(OUT_DIR):
            os.mkdir(OUT_DIR)
        print('--> Export RKNN model: {}'.format(RKNN_MODEL_PATH))
        ret = rknn.export_rknn(RKNN_MODEL_PATH, cpp_gen_cfg=True)
        if ret != 0:
            print('Export rknn model failed.')
            exit(ret)
        print('done')
    else:
        ret = rknn.load_rknn(RKNN_MODEL_PATH)

    rknn.release()

