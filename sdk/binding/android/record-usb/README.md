奇梦者 Android 录音 SDK 集成文档
--

&emsp;&emsp;奇梦者录音机分成 `系统录音机` 和 `USB录音机` 两种，两个录音机的实现都是对应如下 `IRecordPresenter` 接口；调用流程同样是初始化、启动、停止、销毁。

```java
public interface IRecordPresenter {
    /**
     * 初始化录音机
     */
    void initRecorder(IRecordOption option) throws Exception;

    /**
     * 开启录音机
     * [listener] 录音数据回调
     */
    boolean startRecorder(OnAudioRecordListener listener);

    /**
     * 停止录音机
     */
    void stopRecorder();

    /**
     * 销毁录音机
     */
    void releaseRecorder();
}
```

## 一、系统录音机

### 1、导入 jar 包

```groovy
implementation files('libs/qdreamer-java.jar')
```

### 2、初始化系统录音机

```java
IRecordPresenter recordPresenter = new AudioRecordPresenter();
recordPresenter.initRecorder(new AudioRecordOption())
```

&emsp;&emsp;`AudioRecordOption` 无参构造函数默认创建的是 单声道、16k、16bit 的录音机；

- source: 录音源，默认是麦克风
- sample: 采样率，默认是 16k
- channel: 通道数，默认是单声道
- format: 编码格式，默认是 16bit

```java
public class AudioRecordOption implements IRecordOption {
    public AudioRecordOption() {
        this(MediaRecorder.AudioSource.MIC, 16000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
    }

    public AudioRecordOption(int source, int sample, int channel, int format) {
        this.source = source;
        this.sample = sample;
        this.channel = channel;
        this.format = format;
    }
}
```

### 3、开始停止录音

```java
// 开始录音
recordPresenter.startRecorder(onAudioRecordListener);
// 停止录音
recordPresenter.stopRecorder();
```

### 4、录音数据回调

&emsp;&emsp;调用 `startRecorder()` 接口开始录音后，通过设置的回调接口，返回原始 pcm 格式的录音数据。

```java
public interface OnAudioRecordListener {
    void onRecordAudio(byte[] audio);
}
```

### 5、销毁录音机

&emsp;&emsp;录音机用完之后，需要调用 `releaseRecorder()` 接口释放录音机，否则占用后别的应用无法使用录音机录音

```java
recordPresenter.releaseRecorder();
```

## 二、USB 录音机

&emsp;&emsp;USB 录音机和系统录音机的使用方法基本一致，只是导入依赖和初始化环节有点区别，其余代码一样

### 1、初始化录音机

```java
// 注意参数格式，别少了分号；
private static final String SESSION_CONFIG = "appid=0;secretkey=0;cache_path=%s;";

// cache_path 是用于存储 qvoice.log 的，方便调试过程中排查问题；
String sessionConfig = String.format(SESSION_CONFIG, getFilesDir().getAbsolutePath());
IRecordPresenter recordPresenter = new QAudioRecordPresenter()
recordPresenter.initRecorder(QAudioRecordOption(audioCfg.getAbsolutePath(), sessionConfig))
```

&emsp;&emsp;USB 录音机是由 C 实现，通过 JNI 接口在 Android 层调用；因此除了 jar 包提供，还有 libQvoice.so 库文件；
so 动态库中通过传入的 audioCfg 参数读取 audio.cfg 配置文件，因此需要将 audio.cfg 拷贝到本地磁盘中有文件访问权限的路径下。

### 2、audio.cfg 说明

&emsp;&emsp;USB 录音是通过声卡节点录音，因此在使用前也可以先通过 tinycap 命令验证该声卡节点是否可以正常录音

```bash
adb shell
su
chmod 777 /dev/snd/pcmC1D0c
tinycap /sdcard/test.wav -D 1 -d 0 -r 16000 -c 1 -b 16
```

&emsp;&emsp;上面的 tinycap 命令表示使用 /dev/snd/pcmC1D0c 节点录制单通道、16k、16bit 音频数据，并保存到 /sdcard/test.wav 文件中。

&emsp;&emsp;如果 tinycap 录音正常，此时再使用 QAudioRecordPresenter 进行 USB 录音，如果不能正常录音，请检查 USB 设备是否异常

&emsp;&emsp;audio.cfg 配置中可配置参数如下，snd_name 和 device_number 对应声卡节点，示例中对应的是 /dev/snd/pcmC1D0c，
channel 表示录制通道数，sample_rate 表示采样率

```
...
recorder={
	snd_name="1";
	device_number="0";
	buf_time=32;
	#skip_channels=[4,5];
	channel=1;
	sample_rate=16000;
	bytes_per_sample=2;
	use_for_bfio=0;
};
...
```

&emsp;&emsp;示例代码请参考 [RecordActivity](./src/main/java/com/qdreamer/recordusb/RecordActivity.kt)

