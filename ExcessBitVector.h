#ifndef __EXCESS_BITVECTOR__H__
#define __EXCESS_BITVECTOR__H__

#include "BitVector.h"

class ExcessBitVector : public BitVector
{
public:
  ExcessBitVector(const char *);
  virtual ~ExcessBitVector();

  uint64_t find_close(uint64_t);

protected:
  virtual void after_push(bool);

  int16_t &_exs;
  IdxLRU<int16_t> _min, _max;
};

#endif
