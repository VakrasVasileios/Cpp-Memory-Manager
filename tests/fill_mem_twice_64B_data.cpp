#include "mem_man_bench.hpp"
#include <stdint.h>

struct data {
  uint64_t arr[8];
};

int
main(int argc, char const* argv[]) {
  int loops = (CHUNK_SIZE / sizeof(data)) * 2;

  for (int i = 0; i < loops; i++)
    auto tmp = memman::make_pointer<data>();

  CSV_PRINT;

  return 0;
}
