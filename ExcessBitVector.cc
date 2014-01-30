#include <iostream>
#include <climits>
#include <string.h>
#include "ExcessBitVector.h"
#include "LRU.h"

ExcessBitVector::ExcessBitVector(const char *fname)
  : BitVector(fname), _exs(*reinterpret_cast<int16_t *>(_pg + 4096 - 18)),
    _min(*this, 4), _max(*this, 4)
{
  updatePageSegments();
}

ExcessBitVector::~ExcessBitVector()
{
}

void
ExcessBitVector::after_push(bool bit)
{
  // pos is zero based
  uint32_t idx = ((_size - 1) / UPPER_BLK) % 64;
  if (0 == (_size - 1) % UPPER_BLK)
  {
    _min[idx] = _max[idx] = _exs = bit ? 1 : -1;
    return;
  }

  if (bit)
  {
    _exs++;
    if (_exs > _max[idx])
    {
      _max[idx] = _exs;
    }
  }
  else
  {
    _exs--;
    if (_exs < _min[idx])
    {
      _min[idx] = _exs;
    }
  }
}

// open is zero based offset
uint64_t
ExcessBitVector::find_close(uint64_t pos)
{
  if (pos + 1 >= _size)
  {
    std::cerr << "open " << pos << " is out of range [0," << _size << ")" << std::endl;
    throw pos;
  }

  uint64_t mask = 1ULL << (pos % LONG_BIT);
  uint32_t bv = pos / LONG_BIT;
  if (0 == (_bv[bv] & mask))
  {
    std::cerr << "pos " << pos << " is not an open" << std::endl;
    throw pos;
  }

  uint32_t last_bv = (_size - 1) / LONG_BIT;
  int32_t excess = 1;
  uint32_t bpos = ++pos % UPPER_BLK;
  if (bpos > 0)
  {
    // pos is in the middle of the upper block, search to the end
    // there 32 uint64_t in an upper block
    uint32_t end_bv = std::min(last_bv, bv + 32 * (1 + bv / 32));
    mask = 1ULL << (pos % LONG_BIT);
    while (bv < end_bv)
    {
      while (mask)
      {
        excess += _bv[bv] & mask ? 1 : -1;
        if (0 == excess)
        {
          return pos;
        }

        pos++;
        mask <<= 1;
      }
      bv++;
      mask = 1ULL;
    }

    if (bv == last_bv)
    {
      while (mask)
      {
        excess += _bv[bv] & mask ? 1 : -1;
        if (0 == excess)
        {
          return pos;
        }

        if (++pos == _size)
        {
          return ULONG_LONG_MAX;
        }
        mask <<= 1;
      }
    }
  }

  // upper blocks to check
  uint32_t idx_ub = pos / UPPER_BLK;
  uint32_t last_ub = (_size - 1) / UPPER_BLK;
  while (idx_ub < last_ub)
  {
    int32_t min = _min[idx_ub];
    if (0 >= (excess + min))
    {
      // that's the block contains the answer
      // one upper block has 32 words,
      bv = idx_ub * 32;
      for (uint32_t r = 0; r < 32; r++, bv++)
      {
        mask = 1ULL;
        while (mask)
        {
          excess += _bv[bv] & mask ? 1 : -1;
          if (0 == excess)
          {
            return pos;
          }

          pos++;
          mask <<= 1;
        }
      }
      return ULONG_LONG_MAX;
    }

    // we need to get the rank of the this block.
    uint32_t rank2 = _ub[2 * idx_ub];
    uint32_t rank3 = _ub[2 * idx_ub + 2];
    excess += 2 * (_ub[2 * idx_ub + 2] - _ub[2 * idx_ub]) - UPPER_BLK;
    pos += UPPER_BLK;
    idx_ub++;
  }

  // this is the last block
  bv = pos / LONG_BIT;
  uint32_t bv_sz =  (_size - 1 + LONG_BIT) / LONG_BIT;
  while (bv < bv_sz)
  {
    mask = 1ULL;
    while (mask)
    {
      excess += _bv[bv] & mask ? 1 : -1;
      if (0 == excess)
      {
        return pos;
      }

      if (++pos == _size)
      {
        return ULONG_LONG_MAX;
      }
      mask <<= 1;
    }
    bv++;
  }

  return ULONG_LONG_MAX;
}
