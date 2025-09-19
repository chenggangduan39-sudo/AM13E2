from ctypes import *
from enum import IntEnum


class ErrLevel(IntEnum):
    DEBUG = 0,
    WARN = 1,
    ERR = 2,


class qtk_var_ii_t(Structure):
    _fields_=[
        ("theta",c_int),
        ("phi",c_int)
    ]


class qtk_var_str_t(Structure):
    _fields_=[
        ("data",c_char_p),
        ("len",c_int)
    ]


class qtk_var_fi_t(Structure):
    _fields_=[
        ("theta",c_float),
        ("on",c_int)
    ]


class qtk_var_v_t(Union):
    _fields_=[
        ("i",c_int),
        ("f",c_float),
        ("s",c_short),
        ("ii",qtk_var_ii_t),
        ("str",qtk_var_str_t),
        ("fi",qtk_var_fi_t),
    ]


class qtk_var_t(Structure):
    _fields_ = [
        ("type",c_int),
        ("v",qtk_var_v_t),
    ]


class QTK:
    # please refer to sdk/qtk_api.h
    varType = [
        ('SPEECH_START'),
        ('SPEECH_DATA_OGG'),
        ('SPEECH_DATA_PCM'),
        ('SPEECH_END'),
        ('AEC_WAKE'),
        ('AEC_DIRECTION'),
        ('AEC_WAKE_INFO'),
        ('AEC_SLEEP'),
        ('AEC_CANCEL'),
        ('AEC_CANCEL_DATA'),
        ('AEC_WAKE_ONESHOT'),
        ('AEC_WAKE_NORMAL'),
        ('AEC_THETA_HINT'),
        ('AEC_THETA_BF_BG'),
        ('AEC_THETA_BG'),
        ('TTS_START'),
        ('TTS_DATA'),
        ('TTS_END'),
        ('ASR_TEXT', lambda e: e.contents.v.str.data[:e.contents.v.str.len].decode("utf-8")),
        ('ASR_HINT'),
        ('ASR_HOTWORD'),
        ('EVAL_TEXT'),
        ('EMDLG_TEXT'),
        ('VAR_ERR'),
        ('VAR_SOURCE_AUDIO'),
        ('AUDIO_LEFT'),
        ('AUDIO_ARRIVE'),
        ('AUDIO_ERROR')
    ]

    @classmethod
    def init_clib(cls, lib):
        if hasattr(cls, 'qvoice'):
            return
        cls.qvoice = cdll.LoadLibrary(lib)
        cls.session_init_ = cls.qvoice.qtk_session_init
        cls.session_init_.argtypes = [c_char_p,c_int,c_void_p,c_void_p]
        cls.session_init_.restype = c_void_p

        cls.session_exit_ = cls.qvoice.qtk_session_exit
        cls.session_exit_.argtypes = [c_void_p]
        cls.session_exit_.restype = None

        cls.engine_new_ = cls.qvoice.qtk_engine_new
        cls.engine_new_.argtypes = [c_void_p,c_char_p]
        cls.engine_new_.restype = c_void_p

        cls.engine_start_ = cls.qvoice.qtk_engine_start
        cls.engine_start_.argtypes = [c_void_p]
        cls.engine_start_.restype = c_int

        cls.engine_feed_ = cls.qvoice.qtk_engine_feed
        cls.engine_feed_.argtypes = [c_void_p,c_char_p,c_int,c_int]
        cls.engine_feed_.restype = c_int

        cls.engine_set_notify_ = cls.qvoice.qtk_engine_set_notify
        cls.engine_set_notify_.argtypes = [c_void_p,c_void_p,c_void_p]
        cls.engine_set_notify_.restype = None

        cls.engine_delete_ = cls.qvoice.qtk_engine_delete
        cls.engine_delete_.argtypes = [c_void_p]
        cls.engine_delete_.restype = c_int

        cls.engine_reset_ = cls.qvoice.qtk_engine_reset
        cls.engine_reset_.argtypes = [c_void_p]
        cls.engine_reset_.restype = c_int

    def get_err_handler(self):
        @PYFUNCTYPE(None,c_void_p,c_int,c_char_p)
        def _cb(upval, err_id, err_msg):
            print(f'Get Err: {err_id}, {err_msg.decode("utf-8")}')
        self._err_cb = _cb # avoid gc colloect _cb
        return _cb

    def get_engine_handler(self):
        @PYFUNCTYPE(None,c_void_p,c_void_p)
        def _cb(user_data, var):
            e = cast(var,POINTER(qtk_var_t))
            dispatcher = QTK.varType[e.contents.type]
            if isinstance(dispatcher, str):
                self.cb(dispatcher, None)
            else:
                self.cb(dispatcher[0], dispatcher[1](e))
        self._engine_cb = _cb  # avoid gc colloect _cb
        return _cb

    def session_init(self, params, level):
        self.session = QTK.session_init_(c_char_p(params.encode()), c_int(level), None, self.get_err_handler())

    def session_exit(self):
        QTK.session_exit_(self.session)

    def engine_new(self, params):
        self.engine = QTK.engine_new_(self.session, c_char_p(params.encode()))

    def engine_start(self):
        QTK.engine_start_(self.engine)

    def engine_reset(self):
        QTK.engine_reset_(self.engine)

    def engine_delete(self):
        QTK.engine_delete_(self.engine)

    def engine_set_callback(self, cb):
        self.cb = cb
        QTK.engine_set_notify_(self.engine, None, self.get_engine_handler())

    def engine_feed(self, data):
        QTK.engine_feed_(self.engine, c_char_p(data), len(data), 0)

    def engine_feed_end(self):
        QTK.engine_feed_(self.engine, None, 0, 1)
