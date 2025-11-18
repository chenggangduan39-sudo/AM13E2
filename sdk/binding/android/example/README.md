奇梦者 SDK 集成文档（Android）
---


### 一、SDK 文件说明

|        文件名        |            说明             |
|:-----------------:|:-------------------------:|
| libs/qdreamer.jar | Android 层接口，用于调用 C 层 so 库 |
| src/main/jniLibs  |  对应平台的动态库，名称 libQvoie.so  |
|  src/main/assets  |      资源文件，需要拷贝到磁盘目录       |

#### &emsp;1、导入 jar 依赖

~~~groovy
dependencies {
    implementation files('libs/qdreamer.jar')
}
~~~

#### &emsp;2、限定 abi 平台 so

&emsp;&emsp;将 `jniLibs` 文件夹放在应用模块的 `src/main/` 目录下，并在应用模块 `build.gradle` 中限制平台架构：

~~~groovy
defaultConfig {
    ndk {
        abiFilters 'armeabi-v7a', 'arm64-v8a'
    }
}
~~~



### 二、SDK 接口调用流程

![](https://gitee.com/Jin-Yx/image-store/raw/master/img_flow.png)

&emsp;&emsp;Android 层运行流程如上图，对外暴露的接口定义见下方代码；使用示例可参考 [示例 Demo](./src/main/java/com/qdreamer/example/activity/RecordActivity.java)

~~~java
public interface IQVoicePresenter {

    /**
     * 设置 QSession 和 QEngine 的监听器
     */
    void setOnQVoiceCallback(OnQVoiceCallback listener);

    /**
     * 初始化 QSession 和 相关的 QEngine
     */
    void initQEngine() throws Exception;

    /**
     * 启动 QEngine SDK
     */
    boolean startQEngine();

    /**
     * 如果不使用内置录音机；启动之后往 QEngine 当中传输音频数据
     */
    void feedQEngine(byte[] audio);

    /**
     * 停止 QEngine SDK
     */
    boolean stopQEngine();

    /**
     * 销毁释放资源
     */
    void releaseQEngine();
}
~~~

#### &emsp;2.1、拷贝资源文件

&emsp;&emsp;SDK 依赖资源文件，需要将资源文件拷贝到本地磁盘中，可以直接通过 `adb push` 资源到本地；
也可以通过 jar 包中的工具类 `CopyEngineResUtils` 将资源从 `assets` 目录拷贝到本地磁盘；

~~~java
File sdkDir = new File(getExternalFilesDir(null), "sdk");
try {
    // 用于将 assets 下的所有文件拷贝到 /sdcard/Android/data/${applicationId}/files/sdk/ 文件夹下
    CopyEngineResUtils.copyAssets2Local(this, "", sdkDir);
} catch (Exception e) {
    // 资源拷贝失败
}
~~~

#### &emsp;2.2、初始化参数

&emsp;&emsp;SDK 初始化参数分为 session、engine、record 三部分；其中 `session` 用于鉴权处理，通过 appId、secretKey 校验是否有对应语音功能权限；
`engine` 参数为真正的语音功能，通过指定 role 和 cfg 配置权限实现；`record` 为内置录音机，传 null 或不传不创建内置录音机。

~~~java
private static final String SESSION_CONFIG = "appid=%s;secretkey=%s;cache_path=%s;";
private static final String ENGINE_CONFIG = "role=替换你的语音角色;cfg=%s;";

File configFile = new File(sdkDir, "替换你的配置文件名");
if (!configFile.exists() || !configFile.isFile()) {
    // 资源文件不存在，没拷贝成功或者文件名称错误
} else {
    String sessionConfig = String.format(SESSION_CONFIG, "替换你的 appId", "替换你的 secretKey", sdkDir.getAbsolutePath());
    String engineConfig = String.format(ENGINE_CONFIG, configFile.getAbsolutePath());
    IRecordOption recordOption = initRecordOption();
}
~~~

#### &emsp;2.3 内置录音机

&emsp;&emsp;内置录音机分为 `系统录音机` 和 `USB 录音机` 两种

- **系统录音机**

~~~java
// 默认无参构造函数创建的是 MIC 录制的单通道、16k采样率、16bit的 pcm 音频
IRecordOption recordOption = new AudioRecordOption();
IRecordOption recordOption = new AudioRecordOption(MediaRecorder.AudioSource.MIC, 16000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
~~~

- **USB 录音机**

&emsp;&emsp;系统录音机通过传入 audio.cfg 配置进行录音，audio.cfg 中指定录音声卡节点以及音频参数；对应声卡节点的权限需要上层赋予可读可写可执行权限

~~~java
File audioCfg = new File(sdkDir, "audio.cfg");
IRecordOption recordOption = new QAudioRecordOption(audioCfg.getAbsolutePath(), null);
~~~

#### 2.4、创建实例对象并初始化

~~~java
IQVoicePresenter qVoicePresenter = new QVoicePresenter(sessionConfig, engineConfig, recordOption);
qVoicePresenter.setOnQVoiceCallback(this);
qVoicePresenter.initQEngine();
~~~

#### 2.5、开始/停止语音功能

&emsp;&emsp;在使用内置录音机时，上层不需要处理音频数据，而未使用内置录音机，由上层提供音频来源时，需要在开始语音功能之后调用 `feedQEngine(byte[])` 接口传入音频数据

~~~java
if (isRecording) {  // 停止
    qVoicePresenter.stopQEngine();
    btnSDK.setText("开始");
    isRecording = false;
} else {    // 开始
    qVoicePresenter.startQEngine();
    btnSDK.setText("停止");
    isRecording = true;
}
~~~

&emsp;&emsp;3、退出页面的时候需要调用 `releaseQEngine()` 停止录音、语音等功能，释放资源

~~~java
@Override
protected void onDestroy() {
    if (qVoicePresenter != null) {
        qVoicePresenter.releaseQEngine();
        qVoicePresenter = null;
    }
    super.onDestroy();
}
~~~

### 三、SDK 接口回调说明

&emsp;&emsp;数据回调通过 `OnQVoiceCallback` 接口返回，参数 `type` 表示数据类型；参数 `data` 为对应 `type` 携带的数据(可能为空)，注意：回调线程中请勿做好似处理，以免影响后续消息的接收。

~~~java
public interface OnQVoiceCallback {

    /**
     * SDK 结果回调
     * @param type 消息类型
     * @param data 消息数据，data 可能为 null
     */
    void onQVoiceCallback(byte type, byte[] data);

}
~~~


|            类型             |                       说明                     |
| :-------------------------: |       :-----------------------------:         |
| QTK_JNI_INIT_SUCCESS（121） |                 初始化成功回调                 |
| QTK_JNI_INIT_FAILED（122）  |        初始化失败，data 为对应的失败原因        |
|   QTK_JNI_ERRCODE（100）    |       SDK 错误码信息，data 为错误信息 json      |
|  RECORD_AUDIO_DATA（120）   | 使用内置录音时音频数据回调；data 为 pcm 音频数据 |
|             ...             |              对应语音功能消息回调              |

