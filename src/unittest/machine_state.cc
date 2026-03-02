#include "machine_state.hpp"

int main() {
  iris::machine_info info;
  iris::get_machine_info(info);
  auto info_json = info.to_json();
  printf("%s", info_json.c_str());
  return 0;
}