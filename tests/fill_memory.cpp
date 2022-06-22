#include "../mem_man.hpp"
#include <stdint.h>

int
main(int argc, char const* argv[]) {
  int loops = CHUNK_SIZE / sizeof(uint64_t);

  for (int i = 0; i < loops; i++)
    auto tmp = memman::make_pointer<uint64_t>(i);

  return 0;
}
