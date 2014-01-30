#ifndef __BITVECTOR__H__
#define __BITVECTOR__H__

#include <stdint.h>
#include <tr1/memory>
#include <vector>

#include "LRU.h"

class BitVector
{
public:
  friend class BitLRU;
  template<typename T> friend class IdxLRU;

  static const uint64_t BASIC_BLK, UPPER_BLK, SECTION_BLK, SELECT_BLK;
  static const uint32_t TEN_BITS, MASK_10BIT;

  BitVector(const char *);
  virtual ~BitVector();

  const uint64_t push_back(bool);
  const bool at(uint64_t);
  uint64_t size();
  uint64_t rank1(uint64_t);
  uint64_t rank0(uint64_t);
  uint64_t select1(uint64_t);
  uint64_t select0(uint64_t);

protected:
  virtual void before_push();
  virtual void after_push(bool);

  void updatePageSegments();

  int _idx_fd, _data_fd;
  uint8_t *_pg, _pages, _working_pages;
  uint64_t &_size, &_ones, *_working;
  uint32_t _segs, _remainning;
  std::vector<const Segment *> _grp;
  std::vector<std::tr1::shared_ptr<uint8_t> > _idx_pages;

  // there are 4 upper blocks
  // each upper block has 4 basic block,
  // each basic block has 8 uint64_t words
  IdxLRU<uint32_t> _ub, _sb1, _sb0;

  // bit vector
  BitLRU _bv;
};

#endif
