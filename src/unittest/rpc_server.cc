#include <uv.h>

int main() {
  auto loop = uv_loop_new();
  return uv_run(loop, uv_run_mode::UV_RUN_DEFAULT);
}