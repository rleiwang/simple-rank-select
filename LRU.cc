#include <climits>
#include <string.h>

#include "LRU.h"
#include "BitVector.h"

const uint32_t Segment::SIZE = 128U;

static const uint32_t PAGE_SIZE = 4096U;
static const uint32_t HALF_PAGE_SIZE = 2048U;

static const uint32_t BYTES_IN_WORD = LONG_BIT / CHAR_BIT;
static const uint32_t WORDS_IN_HALF_PAGE = HALF_PAGE_SIZE / BYTES_IN_WORD;
static const uint32_t WORDS_IN_PAGE = PAGE_SIZE / BYTES_IN_WORD;

BitLRU::BitLRU(BitVector &bv)
  :_bv(bv)
{
}

BitLRU::~BitLRU()
{
}

uint32_t
BitLRU::copySegment(uint8_t *bv, uint32_t segs)
{
  uint32_t offset = WORDS_IN_HALF_PAGE;
  if (0 == segs % 2)
  {
    _pages.push_back(std::tr1::shared_ptr<uint64_t>(new uint64_t[2 * WORDS_IN_HALF_PAGE]));
    offset = 0;
  }
  memcpy(_pages.back().get() + offset, _bv._pg, HALF_PAGE_SIZE);
  return ++segs;
}

uint64_t&
BitLRU::operator [](uint64_t pos)
{
  uint32_t wcnt = 1 + (_bv._size - 1) / LONG_BIT;
  if ((wcnt - pos) < WORDS_IN_HALF_PAGE)
  {
    return reinterpret_cast<uint64_t *>(_bv._pg)[pos % WORDS_IN_HALF_PAGE];
  }
  return _pages[pos / WORDS_IN_PAGE].get()[pos % WORDS_IN_PAGE];
}

template <typename T>
IdxLRU<T>::IdxLRU(BitVector &bv, uint32_t pages)
    : Segment(pages, Segment::SIZE/sizeof(T), bv._grp.size()), _bv(bv)
{
  _bv._grp.push_back(this);
}

template <typename T>
T&
IdxLRU<T>::operator [](uint32_t pos)
{
  uint32_t seg_idx = pos / num;
  seg_idx *= pages;
  uint32_t num_segs = 0;
  for (std::vector<const Segment *>::const_iterator ci = _bv._grp.begin(); ci != _bv._grp.end(); ++ci)
  {
    if (0 == seg_idx % (*ci)->pages && (*ci)->offset < offset)
    {
      num_segs++;
    }
    if (seg_idx > 0)
    {
      num_segs += (seg_idx - 1 + (*ci)->pages) / (*ci)->pages;
    }
  }

  if (seg_idx / _bv._working_pages == (_bv._segs / 2) / _bv._working_pages)
  {
    return reinterpret_cast<T *>(_bv._pg + 2048 + (num_segs % _bv._pages) * Segment::SIZE)[pos % num];
  }

  return reinterpret_cast<T *>(_bv._idx_pages[num_segs / 32].get() + (num_segs % 32) * Segment::SIZE)[pos % num];
}

template class IdxLRU<uint32_t>;
template class IdxLRU<int16_t>;
