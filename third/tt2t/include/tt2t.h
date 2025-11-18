#ifndef D259FC69_8B3D_3BE5_2A08_E2BD5B64DD4B
#define D259FC69_8B3D_3BE5_2A08_E2BD5B64DD4B

#include <string>

namespace tt2t {
class TT2T {
public:
  static constexpr int N = 100;
  static constexpr int max_limit_len = N - 1;
  TT2T(std::string res_path);
  ~TT2T();
  std::string process(std::string input);

private:
  void *impl_;
};
} // namespace tt2t

#ifdef __cplusplus
extern "C" {
#endif

typedef void* qtt2t_t;

qtt2t_t tt2t_init(char *fn);
char *tt2t_process(qtt2t_t tt2t, char *input, int len);

#ifdef __cplusplus
};
#endif
#endif /* D259FC69_8B3D_3BE5_2A08_E2BD5B64DD4B */
