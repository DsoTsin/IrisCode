#include "handle.h"

using namespace iris;
/**
Master ------------------------------- Slave
   |          createProc [RPC]           |
   *------------------------------------>| local create process in memory (If succeeded, get master
handle) |                                     | return real handle (slave)? or generate fake unique
handle from master |                                     | build slave real handle map <-> remote
handle |                                     | |                                     |
* Useful API is:
*	[[Slave]]
*	handle lookup_remote(HANDLE local);
*	[[Slave]]
*   HANDLE lookup_local(handle remote);
*	[[Slave]]
*	void add_handle(HANDLE local, handle remote);
*	  ---> handle get_remote_handle();
*	[[Slave]]
*	void remove_handle(HANDLE local, handle remote);
*	  ---> free_remote_handle(handle remote);
*	[[Master]] handle value should be different from windows!!
*	handle new();
*	[[Master]]
*	void free(handle in);
*/
int main(int argc, char* argv[]) {
  auto file = handle::new_file();
  auto proc = handle::new_process();
  handle::free(file);
  {
    auto f = handle::new_file();
    handle::free(f);
  }
  auto p0 = handle::new_process();
  auto p1 = handle::new_process();
  handle::free(proc);
  handle::free(p0);
  handle::free(p1);
  return 0;
}