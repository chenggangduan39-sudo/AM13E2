package com.qdreamer.qvoice.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * @Author: JinYx
 * @Create: 2023-12-15 16:02:11
 * @Signature: 不属于这个时代的愚者；灰雾之上的神秘主宰；执掌好运的黄黑之王。
 */
public class WavUtil {

    public static void pcm2Wav(String pcmPath, String wavPath, int sampleRate, short channels) throws Exception {
        pcm2Wav(new File(pcmPath), new File(wavPath), sampleRate, channels);
    }

    public static void pcm2Wav(File pcmFile, File wavFile, int sampleRate, short channels) throws Exception {
        byte[] pcmAudio = getPcmData(pcmFile);
        byte[] wavHeader = getWavHeader(pcmAudio.length, sampleRate, channels);
        FileOutputStream outputStream = new FileOutputStream(wavFile);
        outputStream.write(wavHeader);
        outputStream.write(pcmAudio);
        outputStream.close();
    }

    private static byte[] getPcmData(File pcmFile) throws Exception {
        FileInputStream inputStream = new FileInputStream(pcmFile);
        byte[] data = new byte[(int) pcmFile.length()];
        inputStream.read(data);
        inputStream.close();
        return data;
    }

    private static byte[] getWavHeader(int pcmAudioLen, int sampleRate, short channels) throws Exception {
        char[] riffTag = "RIFF".toCharArray();
        int fileLength = pcmAudioLen + 36;
        char[] wavTag = "WAVE".toCharArray();
        char[] fmtId = "fmt ".toCharArray();
        int fmtLength = 16;
        short format = 1;
        short bitsPerSample = 16;
        int averageBytesPerSec = channels * bitsPerSample / 8 * sampleRate;
        short blockAlign = (short) (channels * bitsPerSample / 8);
        char[] dataId = "data".toCharArray();

        ByteBuffer headers = ByteBuffer.allocate(44);
        headers.order(ByteOrder.LITTLE_ENDIAN);
        for (char riff : riffTag) {
            headers.put((byte) riff);
        }
        headers.putInt(fileLength);
        for (char wav : wavTag) {
            headers.put((byte) wav);
        }
        for (char fmt : fmtId) {
            headers.put((byte) fmt);
        }
        headers.putInt(fmtLength);
        headers.putShort(format);
        headers.putShort(channels);
        headers.putInt(sampleRate);
        headers.putInt(averageBytesPerSec);
        headers.putShort(blockAlign);
        headers.putShort(bitsPerSample);
        for (char data : dataId) {
            headers.put((byte) data);
        }
        headers.putInt(pcmAudioLen);
        return headers.array();
    }

}
