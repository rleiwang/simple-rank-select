#include <iostream>
#include <fstream>

#include "ExcessBitVector.h"
#include "WaveletSequence.h"

int
main(int argc, char **argv) {
  ExcessBitVector bv("bv1");
  for (uint32_t i = 0; i < 4096*8*8; i++)
  {
    bv.push_back(i % 2);
  }
  std::cout << bv.rank1(8) << std::endl;
  std::cout << bv.rank0(8) << std::endl;
  std::cout << "at " << bv.at(7) << std::endl;
  std::cout << "at " << bv.at(8) << std::endl;
  std::cout << bv.select1(10) << std::endl;
  std::cout << bv.select1(4096) << std::endl;
  std::cout << bv.select0(4096) << std::endl;
  std::cout << bv.select1(4096 * 2) << std::endl;
  std::cout << bv.select0(4096 * 2) << std::endl;
  std::cout << bv.select1(4096 * 3) << std::endl;
  std::cout << bv.select0(4096 * 3) << std::endl;
  std::cout << bv.select1(4096 * 8) << std::endl;
  std::cout << bv.select0(4096 * 8) << std::endl;
  std::cout << bv.find_close(59) << std::endl;
  try
  {
  std::cout << bv.rank1(64) << std::endl;
  }
  catch (...)
  {
    std::cout << "got exception " << std::endl;
  }

  std::cout << "before create" << std::endl;
  ExcessBitVector ebv2("bv2");
  ebv2.push_back(1);
  uint64_t last_stop = (1ULL << 11);
  for (uint32_t g = 0; g < last_stop; g++)
  {
    ebv2.push_back(g % 4 > 1 ? 0 : 1);
  }
  ebv2.push_back(0);
  std::cout << "start" << std::endl;
  std::cout << ebv2.find_close(0) << " size is " << last_stop + 2 << std::endl;

  WaveletSequence ws;
  std::ifstream iff("l5.txt");
  while (iff)
  {
    char t;
    iff >> t;
    ws.push_back(t);
  }
}
