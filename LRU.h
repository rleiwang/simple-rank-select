#ifndef __LRU__H__
#define __LRU__H__

#include <stdint.h>
#include <tr1/memory>
#include <vector>

class BitVector;

class BitLRU
{
public:
  BitLRU(BitVector &);
  ~BitLRU();

  uint32_t copySegment(uint8_t *, uint32_t);
  uint64_t& operator[](uint64_t);

private:
  BitVector &_bv;
  std::vector<std::tr1::shared_ptr<uint64_t> > _pages;
};

struct Segment
{
  static const uint32_t SIZE;

  const uint32_t pages;
  const uint32_t num;
  const uint32_t offset;
  Segment(uint32_t pgs, uint32_t num, uint32_t offset) : pages(pgs), num(num), offset(offset) {}
};

template <typename T>
class IdxLRU : public Segment
{
public:
  IdxLRU(BitVector &bv, uint32_t pages);
  ~IdxLRU() {}

  T& operator[](uint32_t pos);
private:
  BitVector &_bv;
};

#endif
