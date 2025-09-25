凯硕达识别：
配置换环境
xmake f -c --with-sdk=yes --sdk-use-asr=yes --with-asr=yes --mode=debug

编辑库文件： xmake build -v qtk
编辑可执行文件： xmake build -v sdk_csr_hjp

用第三方编辑工具链 （交叉编辑）
配置环境 (xmake工具链 设置可参考   https://zhuanlan.zhihu.com/p/96031862?utm_id=0)

xmake f -c --with-sdk=yes --sdk-use-asr=yes --with-asr=yes --mode=release -p linux --sdk=/home/ml/tools/r528/r528-toolchain-sunxi-glibc/toolchain-sunxi-glibc/toolchain --kind=shared

编辑库文件： xmake build -v qtk
编辑可执行文件： xmake build -v sdk_csr_hjp
