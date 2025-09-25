# 轻量级Actor模型实现

- qtk_actor_context_t指代一个actor实体，不同context之间相互独立，各自完成独立的功能，不同context间不共享内存，context间的通讯通过 send msg 来实现，功能通过msg的处理函数实现。

- context内功能实现抽象为一个状态机，通过接受到的不同msg触发不同的行为，状态的切换通过替换context的msg处理函数

# Actor的调度

- 调度器实现为一个线程池，qtk_actor_scheduler_new的入参控制线程的数量

- 同一时刻最多只会有一个线程拿到一个context，所以cotext内部数据无需考虑多线程同步问题

# msg protocol

- 通过约束消息的定义来规范化Actor或Actor Group的输入输出，只要输入，输出msg兼容即可以自由组合实现不同的功能。如一个识别的Actor的输入是Audio的msg产生一个AsrResult的Msg，只要能够产生Audio msg的Actor都可以为识别的Actor提供输入，如AudioRecorder, WavFileReader, 阵列降噪模块。

# 示例代码

```c
char *vad_fn = "/tmp/vad.cfg";
char *asr_fn = "/tmp/asr.cfg";

typedef struct {
// Actor内部状态无需做线程同步
  wtk_vad_cfg_t *cfg;
  wtk_vad2_t *vad;
  unsigned speech : 1;
} Vad; // Vad Actor 输入msg为无切分的Audio, 输出为带边界的Audio， len = 0 的Audio作为语音的开始即结束

typedef struct {
  wtk_main_cfg_t *cfg;
  qtk_wenet_wrapper_t *asr;
} Asr; // Asr Actor 输入为带边界的Audio，与Vad Actor的输出兼容，所以可以组合起来

static void on_vad2_(Vad *v, wtk_vad2_cmd_t cmd, short *data, int len) {
  switch (cmd) {
  case WTK_VAD2_START:
      v->speech = 1;
    qtk_actor_scheduler_send("vad", "asr", NULL, 0); // len = 0代表语音开始
    break;
  case WTK_VAD2_DATA:
      if (v->speech) {
          qtk_actor_scheduler_send("vad", "asr", cast(char *, data), len * 2); // 驱动Asr Actor
      }
    break;
  case WTK_VAD2_END:
      v->speech = 0;
    qtk_actor_scheduler_send("vad", "asr", NULL, 0); // len = 0 代表语音结束，与vad start通过接受时Asr Actor的不同状态作区分
    break;
  case WTK_VAD2_CANCEL:
    break;
  }
}

static Vad *vad_new_(char *cfg) {
  Vad *v = wtk_malloc(sizeof(Vad));
  v->cfg = wtk_vad_cfg_new(cfg);
  v->vad = wtk_vad2_new(v->cfg);
  wtk_vad2_set_notify(v->vad, v, cast(wtk_vad2_notify_f, on_vad2_));
  v->speech = 0;
  return v;
}

static void vad_delete_(Vad *v) {
  wtk_vad2_delete(v->vad);
  wtk_vad_cfg_delete(v->cfg);
  wtk_free(v);
}

static int asr_dispatcher_(Asr *a, qtk_actor_msg_t *msg, void **new_dispatcher);

// vad Actor的msg 处理函数
static int vad_dispatcher_(Vad *v, qtk_actor_msg_t *msg, void **new_dispatcher) {
  wtk_vad2_feed(v->vad, msg->data, msg->len, 0);
  return 0;
}

static int asr_dispatcher_data_(Asr *a, qtk_actor_msg_t *msg, void **new_dispatcher) {
  if(msg->len == 0) {
    wtk_string_t v;
    *new_dispatcher = asr_dispatcher_; // 切换会初始状态
    qtk_wenet_wrapper_feed(a->asr, NULL, 0, 1);
    qtk_wenet_wrapper_get_result(a->asr, &v);
    wtk_debug("%.*s\n", v.len, v.data);
    qtk_wenet_wrapper_reset(a->asr);
  } else {
    qtk_wenet_wrapper_feed(a->asr, msg->data, msg->len, 0);
  }
  return 0;
}

// Asr Actor的msg处理函数（初始状态）
static int asr_dispatcher_(Asr *a, qtk_actor_msg_t *msg, void **new_dispatcher) {
  if (msg->len != 0) {
    wtk_debug("bug\n");
    return 1;
  }
  qtk_wenet_wrapper_start(a->asr);
  *new_dispatcher = asr_dispatcher_data_; // 修改msg处理函数实现Asr Actor的状态切换（切换至识别中...状态）
  return 0;
}

static Asr *asr_new_(char *cfg) {
  Asr *v = wtk_malloc(sizeof(Asr));
  v->cfg = qtk_wenet_wrapper_cfg_new(cfg);
  v->asr = qtk_wenet_wrapper_new(v->cfg->cfg);
  wtk_debug("%p\n", v->asr);
  return v;
}

static void asr_delete_(Asr *v) {
  qtk_wenet_wrapper_delete(v->asr);
  qtk_wenet_wrapper_cfg_delete(v->cfg);
  wtk_free(v);
}

int main() {
  qtk_actor_scheduler_new(8); // 参数8开启了8个工作线程

  qtk_actor_context_register("vad", vad_fn, cast(qtk_actor_new_f, vad_new_),
                             cast(qtk_actor_delete_f, vad_delete_),
                             cast(qtk_actor_dispatcher_f, vad_dispatcher_));
  qtk_actor_context_register("asr", asr_fn, cast(qtk_actor_new_f, asr_new_),
                             cast(qtk_actor_delete_f, asr_delete_),
                             cast(qtk_actor_dispatcher_f, asr_dispatcher_));

  qtk_actor_scheduler_start();
  wtk_riff_t *riff;

  riff = wtk_riff_new();
  wtk_riff_open(riff, "/home/zlh/tmp/test.wav");
  char buf[2048];
  int len;

  while (1) {
    len = wtk_riff_read(riff, buf, sizeof(buf));
    if (len > 0) {
      qtk_actor_scheduler_send("sys", "vad", buf, len); // 驱动整个系统
    }
    if (len < sizeof(buf)) {
      break;
    }
  }

  qtk_actor_scheduler_stop();
  qtk_actor_context_retire("vad");
  qtk_actor_context_retire("asr");
  qtk_actor_scheduler_delete();
  wtk_riff_delete(riff);
}
```