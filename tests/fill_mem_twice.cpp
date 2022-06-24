#include "mem_man_bench.hpp"
#include <stdint.h>

int
main(int argc, char const* argv[]) {
  int loops = (CHUNK_SIZE / sizeof(uint64_t)) * 2;

  for (int i = 0; i < loops; i++)
    auto tmp = memman::make_pointer<uint64_t>(i);

  CSV_PRINT;

  return 0;
}
