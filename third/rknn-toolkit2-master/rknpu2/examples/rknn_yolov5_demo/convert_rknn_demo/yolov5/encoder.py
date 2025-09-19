
import cv2
import numpy as np

from rknn.api import RKNN
import os

if __name__ == '__main__':

    platform = 'rk3576'
    #platform = 'rk3588'
    # Model from https://github.com/airockchip/rknn_model_zoo
    #MODEL_PATH = 'unet_noise_revb_0113_small.onnx' 
    #MODEL_PATH = 'realesr-general-x4v3.onnx' 
    MODEL_PATH = 'scrfd_0.5_384x640_face.onnx' 
    #MODEL_PATH = 'xlsr-rgb-x2.onnx' 
    NEED_BUILD_MODEL = True
    # NEED_BUILD_MODEL = False

    # Create RKNN object
    rknn = RKNN()

    OUT_DIR = "rknn_models"
    RKNN_MODEL_PATH = 'encoder_model.rknn'
    if NEED_BUILD_MODEL:
        DATASET = './dataset.txt'
        rknn.config(mean_values=[[127.5, 127.5, 127.5]], std_values=[[128, 128, 128]], target_platform=platform)#scrfd_0.5_384x640_face.onnx
        #rknn.config(mean_values=[[0, 0, 0]], std_values=[[255, 255, 255]], target_platform=platform)#realesr-general-x4v3.onnx
        #rknn.config(target_platform=platform)
        # Load model
        print('--> Loading model')
        ret = rknn.load_onnx(MODEL_PATH)
        #ret = rknn.load_onnx(MODEL_PATH, inputs=['input.1'], input_size_list=[[1,3,384,640]])
        #ret = rknn.load_onnx(MODEL_PATH, inputs=['input'], input_size_list=[[1,3,540,960]])
        #ret = rknn.load_onnx(MODEL_PATH, inputs=['input'], input_size_list=[[1,3,360,640]])
        #ret = rknn.load_onnx(MODEL_PATH, inputs=['input'], input_size_list=[[1,3,640,360]])
        #ret = rknn.load_onnx(MODEL_PATH, inputs=['audio_input', 'video_input'], input_size_list=[[1,2,257,256], [1,256,1,64]])
        #ret = rknn.load_onnx(MODEL_PATH, inputs=['audio_input', 'video_input'], input_size_list=[[1,2,257,256], [1,512,1,64]]) #big model
        if ret != 0:
            print('load model failed!')
            exit(ret)
        print('done')

        # Build model
        print('--> Building model')
        ret = rknn.build(do_quantization=False, dataset=DATASET)
        #ret = rknn.build(do_quantization=False)
        if ret != 0:
            print('build model failed.')
            exit(ret)
        print('done')

        # Export rknn model
        if not os.path.exists(OUT_DIR):
            os.mkdir(OUT_DIR)
        print('--> Export RKNN model: {}'.format(RKNN_MODEL_PATH))
        ret = rknn.export_rknn(RKNN_MODEL_PATH)
        if ret != 0:
            print('Export rknn model failed.')
            exit(ret)
        print('done')
    else:
        ret = rknn.load_rknn(RKNN_MODEL_PATH)

    rknn.release()
