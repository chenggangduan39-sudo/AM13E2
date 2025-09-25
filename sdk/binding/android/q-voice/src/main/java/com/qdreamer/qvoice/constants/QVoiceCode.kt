package com.qdreamer.qvoice.constants

object QVoiceCode {

    const val QTK_JNI_SPEECH_START: Byte          = 0
    const val QTK_JNI_SPEECH_DATA_OGG: Byte       = 1
    const val QTK_JNI_SPEECH_DATA_PCM: Byte       = 2
    const val QTK_JNI_SPEECH_END: Byte            = 3
    const val QTK_JNI_AEC_WAKE: Byte              = 4
    const val QTK_JNI_AEC_DIRECTION: Byte         = 5
    const val QTK_JNI_AEC_WAKE_INFO: Byte         = 6
    const val QTK_JNI_AEC_SLEEP: Byte             = 7
    const val QTK_JNI_AEC_CANCEL: Byte            = 8
    const val QTK_JNI_AEC_CANCEL_DATA: Byte       = 9
    const val QTK_JNI_AEC_WAKE_ONESHOT: Byte      = 10
    const val QTK_JNI_AEC_WAKE_NORMAL: Byte       = 11
    const val QTK_JNI_AEC_THETA_HINT: Byte        = 12
    const val QTK_JNI_AEC_THETA_BF_BG: Byte       = 13
    const val QTK_JNI_AEC_THETA_BG: Byte          = 14
    const val QTK_JNI_ASR_DATA: Byte              = 20
    const val QTK_JNI_ASR_HINT: Byte              = 21
    const val QTK_JNI_ASR_HOTWORD: Byte           = 22
    const val QTK_JNI_TTS_START: Byte             = 30
    const val QTK_JNI_TTS_DATA: Byte              = 31
    const val QTK_JNI_TTS_END: Byte               = 32
    const val QTK_JNI_SEMDLG_DATA: Byte           = 40
    const val QTK_JNI_EVAL_DATA: Byte             = 50
    const val QTK_JNI_NORESPONSE: Byte            = 60
    const val QTK_JNI_SOURCE_AUDIO: Byte          = 61
    const val QTK_JNI_AUDIO_LEFT: Byte            = 70
    const val QTK_JNI_AUDIO_ARRIVE: Byte          = 71
    const val QTK_JNI_AUDIO_ERROR: Byte           = 72
    const val QTK_JNI_ERRCODE: Byte               = 100

    const val RECORD_AUDIO_DATA: Byte             = 120
    const val QTK_JNI_INIT_SUCCESS: Byte          = 121
    const val QTK_JNI_INIT_FAILED: Byte           = 122

}