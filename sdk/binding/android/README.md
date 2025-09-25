奇梦者 SDK Android 集成文档
--

- [x] [q-voice](./q-voice): QVoice Android SDK 接口层 —— kotlin 版
- [x] [q-voice-java](./q-voice-java): QVoice Android SDK 接口层 —— java 版
- [x] [record-usb](./record-usb/README.md): 系统录音机 + USB 录音机 使用示例，sdk 中内置集成了录音机
- [x] [example](./example/README.md): Android 层 SDK 集成通用代码，不同语音功能一般只需要替换资源、so、参数配置；Android 层代码不需要变动


注意:  
&emsp;&emsp;如果提供的是 `q-voice` 中的 jar 包，因其使用了 kotlin 协程，需要导入 jar 包时再添加协程依赖。两个 jar 包模块接口实现都一致，一般没特殊要求 kotlin，提供 java 版即可

~~~groovy
implementation "org.jetbrains.kotlinx:kotlinx-coroutines-core:1.3.2"
implementation "org.jetbrains.kotlinx:kotlinx-coroutines-android:1.3.2"
~~~
