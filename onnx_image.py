
import onnxruntime
from PIL import Image
import numpy as np
import cv2
 
# 创建ONNX运行时会话
ort_session = onnxruntime.InferenceSession('res-nnrt/av_model/xlsr-rgb-x2.onnx')
 
# 读取图像
image = Image.open('data_nnrt/00001.jpeg')
input_image = np.array(image)
 
# 对输入图像进行预处理，根据你的模型的需要
# 例如，转换颜色空间，归一化，调整形状等
# 这里需要根据你的模型的具体要求来定
 
# 转换为模型需要的输入格式，通常是NCHW或者NHWC
input_image = np.expand_dims(input_image, 0)
 
# 运行模型进行推断
outputs = ort_session.run(None, {'input': input_image})
 
# 处理输出，根据模型的输出和需求来处理
# 例如，如果输出是一个分类得分，可能需要找到最高分类
# 如果输出是一个图像，可能需要将其转换为图像格式
 
# 保存输出图像
output_image = outputs[0][0]  # 假设模型输出是一个图像
output_image = output_image.astype(np.uint8)  # 转换为uint8类型
 
# 如果输出图像是需要显示的格式，可以直接使用PIL显示
output_image = Image.fromarray(output_image)
output_image.show()
 
# 如果需要保存输出图像
output_image.save('output_image.jpeg')
