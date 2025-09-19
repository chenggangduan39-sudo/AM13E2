package com.qdreamer.qvoice.presenter

import com.qdreamer.qvoice.listener.OnQVoiceCallback

/**
 * 根据目前的使用经验，QVoice 的使用步骤对应如下几个接口
 * - [initQEngine]
 * - [startQEngine]
 * - [feedQEngine]
 * - [stopQEngine]
 * - [releaseQEngine]
 */
interface IQVoicePresenter {

    /**
     * 设置 QSession 和 QEngine 的监听器
     */
    fun setOnQVoiceCallback(listener: OnQVoiceCallback?)

    /**
     * 初始化 QSession 和 相关的 QEngine
     */
    fun initQEngine()

    /**
     * 启动 QEngine SDK
     */
    fun startQEngine(): Boolean

    /**
     * 启动之后往 QEngine 当中传输音频数据
     */
    fun feedQEngine(audio: ByteArray)

    /**
     * 停止 QEngine SDK
     */
    fun stopQEngine(): Boolean

    /**
     * 销毁释放资源
     */
    fun releaseQEngine()

    /**
     * 因为 QVoicePresenter 中封装的是 1 个 QSession + 1 个 QEngine 的模式，但是在真实场景中可能是多个 QEngine 共用同一个 QSession；
     * 所以提供接口获取 session 值和设置 session 值，用于确保创建一个 QSession。
     */
    fun getSessionValue(): Long

}