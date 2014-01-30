#ifndef __WAVELET_SEQUENCE__H__
#define __WAVELET_SEQUENCE__H__

#include <tr1/memory>
#include <vector>

#include "BitVector.h"

enum BitVectorType
{
  SEQ = 0,
  TERM,
  PASS
};

struct SequenceNode
{
  std::tr1::shared_ptr<BitVector> _bvs[3];
};

class WaveletSequence
{
public:
  void push_back(uint8_t);
  bool search(uint8_t, std::vector<uint64_t>&);

private:
  struct SequenceNode _nodes[127];
};

#endif
