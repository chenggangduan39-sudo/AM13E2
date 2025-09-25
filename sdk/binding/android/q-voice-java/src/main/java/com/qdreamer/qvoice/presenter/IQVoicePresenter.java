package com.qdreamer.qvoice.presenter;

import com.qdreamer.qvoice.listener.OnQVoiceCallback;

public interface IQVoicePresenter {

    /**
     * 设置 QSession 和 QEngine 的监听器
     */
    void setOnQVoiceCallback(OnQVoiceCallback listener);

    /**
     * 初始化 QSession 和 相关的 QEngine
     */
    void initQEngine();

    /**
     * 启动 QEngine SDK
     */
    boolean startQEngine();

    /**
     * 启动之后往 QEngine 当中传输音频数据
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

    /**
     * 因为 QVoicePresenter 中封装的是 1 个 QSession + 1 个 QEngine 的模式，但是在真实场景中可能是多个 QEngine 共用同一个 QSession；
     * 所以提供接口获取 session 值，以便创建下一个 QVoicePresenter 实例时可以传参
     */
    long getSessionValue();
}
