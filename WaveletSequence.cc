#include <sstream>
#include <assert.h>
#include "WaveletSequence.h"

static const uint8_t S[] = {4, 2, 1};
/**
 * 1100011
 * 1111111
 *
 * @param x
 * @return
 */
//uint32_t getBitsMask(uint32_t x)
uint8_t
getBitsMask(uint8_t x)
{
  for (int32_t i = 2; i >= 0; --i)
  {
    x |= x >> S[i];
  }
  return x;
}

/**
 * 1100011
 * 1000000
 *
 * @param x
 * @return
 */
uint8_t
leftMostBit(uint8_t x) {
  x = getBitsMask(x);
  return x - (x >> 1);
}

void
WaveletSequence::push_back(uint8_t t)
{
  assert(t > 1);
  uint8_t mask = leftMostBit(t) >> 1;

  uint32_t n = 0;
  BitVectorType e = PASS;
  while (mask)
  {
    if (1 == mask)
    {
      e = TERM;
    }

    uint32_t xe = 0x3 ^ e;
    if (!_nodes[n]._bvs[e].get())
    {
      std::ostringstream  oss;
      oss << "bv_" << n << "." << e;
      _nodes[n]._bvs[e] = std::tr1::shared_ptr<BitVector>(new BitVector(oss.str().c_str()));
      if (_nodes[n]._bvs[xe].get())
      {
        std::ostringstream oss1;
        oss1 << "bv_" << n << "." << SEQ;
        _nodes[n]._bvs[SEQ] = std::tr1::shared_ptr<BitVector>(new BitVector(oss1.str().c_str()));
        bool seq_bit = xe & 0x1;
        for (uint64_t i = 0, cnt = _nodes[n]._bvs[xe]->size(); i < cnt; i++)
        {
          _nodes[n]._bvs[SEQ]->push_back(seq_bit);
        }
      }
    }
    bool bit = mask & t;
    _nodes[n]._bvs[e]->push_back(bit);
    if (_nodes[n]._bvs[SEQ].get())
    {
      _nodes[n]._bvs[SEQ]->push_back(e & 0x1);
    }
    n = 2 * n + 1 + bit;
    mask >>= 1;
  }
}

bool
WaveletSequence::search(uint8_t t, std::vector<uint64_t>& pos)
{
  uint8_t mask = leftMostBit(t) >> 1;

  uint32_t n = 0;
  BitVectorType e = PASS;
  while (mask)
  {

    mask >>= 1;
  }

  return !pos.empty();
}
