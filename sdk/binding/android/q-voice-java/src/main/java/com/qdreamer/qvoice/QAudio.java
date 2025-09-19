package com.qdreamer.qvoice;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;

import java.nio.ByteBuffer;
import java.text.DecimalFormat;

public class QAudio {
	private static AudioTrack player = null; // 播放器
	private static AudioRecord recorder = null; // 录音机

	// ##########################################
	// #
	// # jni接口
	// #
	// ##########################################
	// recorder
	public native long init(long session, String res);

	public native int exit(long audio);

	public native int recorderStart(long audio);

	public native int recorderStop(long audio);

	public native byte[] recorderRead(long audio);
	public native int recorderRead(long audio, ByteBuffer buffer);

	public native int recorderGetChannel(long audio);

	public native int recorderGetRate(long audio);

	public native int recorderGetBytes(long audio);

	// palyer
	private native static int playerStart(long audio);

	private native static int playerStop(long audio);

	private native static int playerSetFormat(long audio, int rate, int channel, int bytes_per_sample);

	private native static int playerPlay(long audio, byte data[], int bytes, int is_end, int syn);

	private native static int playerInterrupt(long audio);

	private native static int playerSetVolume(long audio, float volume);

	private native static int playerIncVolume(long audio);

	private native static int playerDecVolume(long audio);

	private native static int playerSetPitch(long audio, float pitch);

	private native static int payerIncPitch(long audio);

	private native static int playerDecPitch(long audio);

	/**
	 * 计算音量值
	 *
	 * @param buf_time
	 * @param buffer
	 * @return
	 */
	public int calculateVolume(int buf_time, byte[] buffer) {
		double volume = 0.0;
		DecimalFormat df;
		int v = 0;
		for (int i = 0; i < buffer.length; i += 2) {
			int v1 = buffer[i] & 0xFF;
			int v2 = buffer[i + 1] & 0xFF;
			int temp = v1 + (v2 << 8);// 小端
			if (temp >= 0x8000) {
				temp = 0xffff - temp;
			}
			volume += Math.abs(temp);
		}
		volume /= buf_time * 16;
		volume /= 6500;
		volume = volume > 1.0 ? 1.0 : volume;
		df = new DecimalFormat("######0");
		v = Integer.parseInt(df.format(volume * 100));
		return v;
	}

	// ##########################################
	// #
	// # jni交互录音相关接口(系统录音机)
	// #
	// ##########################################
	/**
	 * @Description: 开始录音
	 * @param rate
	 *            录音的采用率
	 * @param buf_time
	 *            录音的缓冲区时间长度(毫秒)
	 * @return
	 */
	private static int RecorderStart(int rate, int channel, int bytes_per_sample, int buf_time) {
		if (recorder == null) {
			int audioEncoding = AudioFormat.ENCODING_PCM_16BIT;
			int audioBufSize = AudioRecord.getMinBufferSize(rate, channel, audioEncoding);
			int size = rate * channel * bytes_per_sample * buf_time / 1000;
			audioBufSize = audioBufSize > size ? audioBufSize : size;
			recorder = new AudioRecord(MediaRecorder.AudioSource.MIC, rate,
					channel == 1 ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.CHANNEL_IN_STEREO, audioEncoding,
					audioBufSize);
		}
		recorder.startRecording();
		return 0;
	}

	/**
	 * @Description 读取录音数据
	 * @param buff
	 * @return
	 */
	private static int RecorderRead(byte[] buff) {
		int len;

		if (recorder != null) {
			len = recorder.read(buff, 0, buff.length);
		} else {
			len = 0;
		}
		return len;
	}

	/**
	 * @Description 停止录音
	 * @return
	 */
	private static int RecorderStop() {
		if (recorder != null) {
			recorder.stop();
			recorder.release();
			recorder = null;
		}
		return 0;
	}

	// ##########################################
	// #
	// # jni交互播放相关接口(系统播放器)
	// #
	// ##########################################
	/**
	 * @Description 语音引擎调用PlayStart，准备播放音频
	 * @param rate
	 *            语音的采样率
	 * @param channel
	 *            语音的频道
	 * @param buf_time
	 *            播放语音的缓冲区时间长度(毫秒)
	 * @return
	 */

	public static int PlayerStart(int rate, int channel, int bytes_per_sample, int buf_time) {
		if (player == null) {
			int buf_size = rate * buf_time * bytes_per_sample * channel / 1000;
			player = new AudioTrack(AudioManager.STREAM_MUSIC, rate,
					channel == 1 ? AudioFormat.CHANNEL_OUT_MONO : AudioFormat.CHANNEL_OUT_STEREO,
					AudioFormat.ENCODING_PCM_16BIT, buf_size, AudioTrack.MODE_STREAM);
		}
		player.play();
		return 0;
	}

	/**
	 * @Description 语音引擎调用PlayWrite，播放音频
	 * @param data
	 *            开始要播放的语音数据
	 * @param len
	 *            要播放的语音数据的有效长度
	 * @return
	 */
	private static int PlayerWrite(byte[] data, int len) {
		int ret;
		if (player != null) {
			ret = player.write(data, 0, len);
		} else {
			ret = 0;
		}
		player.flush();
		return ret;
	}

	/**
	 * @Description 停止播放
	 * @return
	 */
	private static int PlayerStop() {
		if (player != null) {
			player.flush();
			player.stop();
		}
		return 0;
	}

}
