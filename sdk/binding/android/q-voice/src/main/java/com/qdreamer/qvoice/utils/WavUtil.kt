package com.qdreamer.qvoice.utils

import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder

object WavUtil {

    @Throws(Exception::class)
    fun pcm2Wav(pcmPath: String, wavPath: String, sampleRate: Int, channels: Short) {
        pcm2Wav(File(pcmPath), File(wavPath), sampleRate, channels)
    }

    @Throws(Exception::class)
    fun pcm2Wav(pcmFile: File, wavFile: File, sampleRate: Int, channels: Short) {
        val pcmAudio = pcmFile.readBytes()
        wavFile.writeBytes(getWavHeader(pcmAudio.size, sampleRate, channels))
        wavFile.appendBytes(pcmAudio)
    }

    @Throws(Exception::class)
    private fun getWavHeader(pcmAudioLen: Int, sampleRate: Int, channels: Short): ByteArray {
        val fileId: CharArray = "RIFF".toCharArray()
        val fileLength: Int = pcmAudioLen + 36
        val wavTag: CharArray = "WAVE".toCharArray()
        val fmtId: CharArray = "fmt ".toCharArray()
        val fmtLength = 16
        val format: Short = 1
        val bitsPerSample: Short = 16
        val averageBytesPerSec = channels * bitsPerSample / 8 * sampleRate
        val blockAlign: Short = (channels * bitsPerSample / 8).toShort()
        val dataId: CharArray = "data".toCharArray()

        val headers = ByteBuffer.allocate(44)
        headers.order(ByteOrder.LITTLE_ENDIAN)
        for (char in fileId) {
            headers.put(char.toByte())
        }
        headers.putInt(fileLength)
        for (char in wavTag) {
            headers.put(char.toByte())
        }
        for (char in fmtId) {
            headers.put(char.toByte())
        }
        headers.putInt(fmtLength)
        headers.putShort(format)
        headers.putShort(channels)
        headers.putInt(sampleRate)
        headers.putInt(averageBytesPerSec)
        headers.putShort(blockAlign)
        headers.putShort(bitsPerSample)
        for (char in dataId) {
            headers.put(char.toByte())
        }
        headers.putInt(pcmAudioLen)
        return headers.array()
    }

}