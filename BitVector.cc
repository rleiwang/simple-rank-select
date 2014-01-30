#include <climits>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <unistd.h>

// file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
// file

#include "BitVector.h"
#include "LRU.h"

static const int64_t PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
static const int64_t HALF_PAGE_SIZE = PAGE_SIZE / 2;

const uint64_t BitVector::BASIC_BLK = 512;
const uint64_t BitVector::UPPER_BLK = 4 * 512;
const uint64_t BitVector::SELECT_BLK = 8192;
const uint64_t BitVector::SECTION_BLK = 1ULL << 32;

const uint32_t BitVector::TEN_BITS = 10;
const uint32_t BitVector::MASK_10BIT = (1U << TEN_BITS) - 1U;

static int
openFile(const char *name, const char *ext, bool fills =true)
{
  struct stat st;
  int fd = 0;
  std::string fname(name);
  fname.append(ext);
  if (stat(fname.c_str(), &st))
  {
    fd = open(fname.c_str(), O_CREAT|O_RDWR, 0666);
    if (fills)
    {
      // fill zeros
      assert(!ftruncate(fd, PAGE_SIZE));
    }
  }
  else
  {
    fd = open(fname.c_str(), O_RDWR);
  }
  assert(fd > 0);
  return fd;
}

static uint8_t *
mapFile(int fd)
{
  return reinterpret_cast<uint8_t *>(mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
}

BitVector::BitVector(const char *fname)
  : _idx_fd(openFile(fname, ".i")), _pg(mapFile(_idx_fd)), _data_fd(openFile(fname, ".d", false)),
    _working(reinterpret_cast<uint64_t *>(_pg - 8)),
    _size(*reinterpret_cast<uint64_t *>(_pg + PAGE_SIZE - 8)),
    _ones(*reinterpret_cast<uint64_t *>(_pg + PAGE_SIZE - 16)),
    _bv(*this), _ub(*this, 1), _sb1(*this, 8), _sb0(*this, 8), _remainning(0), _segs(0)
{
  updatePageSegments();
}

BitVector::~BitVector()
{
  munmap(reinterpret_cast<void *>(_pg), PAGE_SIZE);
  close(_idx_fd);
  close(_data_fd);
}

uint64_t
BitVector::size()
{
  return _size;
}

void
BitVector::before_push()
{
  uint32_t temp = _size % UPPER_BLK;
  uint32_t idx = 2 * (_size / UPPER_BLK);
  if (0 == temp)
  {
    if (0 == _size % (8 * UPPER_BLK) && _size > 0)
    {
      // save to a new page
      _segs = _bv.copySegment(_pg, _segs);
      pwrite(_data_fd, _pg, HALF_PAGE_SIZE, lseek(_data_fd, 0, SEEK_END));
      memset(_pg, 0, HALF_PAGE_SIZE);
      _working = reinterpret_cast<uint64_t *>(_pg - 8);
      if (0 == _segs % (2 * _working_pages) && _segs > 0)
      {
        // copy index
        uint32_t data_size = _pages * Segment::SIZE;
        while (_remainning < data_size)
        {
          if (_remainning > 0)
          {
            memcpy(_idx_pages.back().get() + PAGE_SIZE - _remainning, _pg + HALF_PAGE_SIZE, _remainning);
            data_size -= _remainning;
          }
          _idx_pages.push_back(std::tr1::shared_ptr<uint8_t>(new uint8_t[PAGE_SIZE]));
          _remainning = PAGE_SIZE;
        }

        if (data_size > 0)
        {
          memcpy(_idx_pages.back().get() + PAGE_SIZE - _remainning,
                 _pg + HALF_PAGE_SIZE + _pages * Segment::SIZE - data_size, data_size);
          _remainning -= data_size;
        }
        pwrite(_idx_fd, _pg + HALF_PAGE_SIZE, _pages * Segment::SIZE, lseek(_idx_fd, 0, SEEK_END));
        memset(_pg + HALF_PAGE_SIZE, 0, _pages * Segment::SIZE);
      }
    }
    _ub[idx] = _ones;
  }
  else if (0 == temp % BASIC_BLK)
  {
    uint32_t shifts = temp / BASIC_BLK - 1;
    uint32_t prev = _ub[idx];
    for (uint32_t r = 0; r < shifts; r++)
    {
      prev += MASK_10BIT & (_ub[idx + 1] >> (TEN_BITS * r));
    }
    _ub[idx + 1] |= (_ones - prev) << (TEN_BITS * shifts);
  }
}

const uint64_t
BitVector::push_back(bool bit)
{
  before_push();
  uint32_t offset = _size++ % LONG_BIT;
  if (0 == offset)
  {
    _working++;
  }

  if (bit)
  {
    *_working |= 1ULL << offset;
    if (1 == ++_ones % SELECT_BLK)
    {
      // position is the offset, 32 bits
      uint32_t temp = static_cast<uint32_t>(_size);
      temp -= (_size & ~((1ULL << 32) - 1)) ? 0 : 1U;
      _sb1[_ones / SELECT_BLK] = temp;
    }
  }
  else
  {
    if (1 == (_size - _ones) % SELECT_BLK)
    {
      // position is the offset, 32 bits
      uint32_t temp = static_cast<uint32_t>(_size);
      temp -= (_size & ~((1ULL << 32) - 1)) ? 0 : 1U;
      _sb0[(_size - _ones) / SELECT_BLK] = temp;
    }
  }
  after_push(bit);
  //msync(_pg, PAGE_SIZE, MS_ASYNC);
  return _size - 1;
}

void
BitVector::after_push(bool bit)
{
}

// pos is zero based offset
const bool
BitVector::at(uint64_t pos)
{
  if (pos > _size)
  {
    std::cerr << "at [" << pos << "] is out of range [0," << _size << ")" << std::endl;
    throw pos;
  }

  // operator precedence [] % << & ?:
  return _bv[pos / LONG_BIT] & (1ULL << pos % LONG_BIT) ? true : false;
}

// pos is zero based offset
uint64_t
BitVector::rank1(uint64_t pos)
{
  if (pos >= _size)
  {
    return ULONG_LONG_MAX;
  }

  // TODO: handle 4G case
  pos = pos % SECTION_BLK;

  // zero based upper block index
  uint32_t idx = pos / UPPER_BLK;

  // rank at the beginning of this super block
  uint32_t rank = _ub[2 * idx];

  // zero based basic block index
  uint32_t bb_idx =  (pos % UPPER_BLK) / BASIC_BLK;
  for (uint32_t r = 1; r <= bb_idx; ++r)
  {
    rank += MASK_10BIT & (_ub[2 * idx + 1] >> ((r - 1) * TEN_BITS));
  }

  idx *= UPPER_BLK / LONG_BIT;
  idx += bb_idx * BASIC_BLK / LONG_BIT;
  bb_idx = pos / LONG_BIT;
  while (idx < bb_idx)
  {
    rank += __builtin_popcountll(_bv[idx]);
    idx++;
  }

  idx = pos % LONG_BIT;
  if (idx > 0)
  {
    uint64_t mask = (1ULL << idx) - 1;
    rank += __builtin_popcountll(_bv[bb_idx] & mask);
  }
  return rank;
}

// return zero based position
uint64_t
BitVector::select1(uint64_t rank)
{
  if (0 == rank || rank > _ones)
  {
    return ULONG_LONG_MAX;
  }

  // pos is zero based
  uint32_t pos = _sb1[rank / (SELECT_BLK + 1)];

  // upper block idx, zero based
  uint32_t idx = pos / UPPER_BLK;

  // last upper blocks idx, zero based
  uint32_t last_ub = (_size - 1) / UPPER_BLK;

  while (idx < last_ub && _ub[2 * (idx + 1)] < rank)
  {
    ++idx;
  }

  // remaing
  rank -= _ub[2 * idx];

  // basic blocks
  uint32_t bb_idx = 0;
  uint32_t last_bb = std::min(3U, static_cast<uint32_t>((_size % UPPER_BLK - 1) / BASIC_BLK));
  while (bb_idx < last_bb)
  {
    // rank of the basic block
    uint32_t temp = MASK_10BIT & (_ub[2 * idx + 1] >> (bb_idx * TEN_BITS));
    if (rank <= temp)
    {
      break;
    }
    rank -= temp;
    ++bb_idx;
  }

  pos = idx * UPPER_BLK + bb_idx * BASIC_BLK;
  assert(rank > 0);

  // this is the basic block, basic block has 8 64bits
  uint32_t bv_idx = (pos / LONG_BIT);
  uint32_t last_bv = (_size - 1) / LONG_BIT;
  for (uint32_t r = 0, cnt = std::min(last_bv, 8U); r < cnt; ++r, ++bv_idx)
  {
    uint32_t temp = __builtin_popcountll(_bv[bv_idx]);
    if (rank <= temp)
    {
      break;
    }
    rank -= temp;
    pos += LONG_BIT;
  }

  // bv_idx contains answer
  uint64_t mask = 1ULL;
  while (mask > 0)
  {
    if ((_bv[bv_idx] & mask) && 0 == --rank)
    {
      break;
    }

    ++pos;
    mask <<= 1;
  }

  return pos;
}

// pos is zero based offset
uint64_t
BitVector::rank0(uint64_t pos)
{
  if (pos >= _size)
  {
    return ULONG_LONG_MAX;
  }

  // TODO: handle 4G case
  pos = pos % SECTION_BLK;

  // zero based upper block index
  uint32_t idx = pos / UPPER_BLK;

  // rank at the beginning of this super block
  uint32_t rank = idx * UPPER_BLK - _ub[2 * idx];

  // zero based basic block index
  uint32_t bb_idx =  (pos % UPPER_BLK) / BASIC_BLK;
  for (uint32_t r = 1; r <= bb_idx; ++r)
  {
    rank += BASIC_BLK - (MASK_10BIT & (_ub[2 * idx + 1] >> ((r - 1) * TEN_BITS)));
  }

  idx *= UPPER_BLK / LONG_BIT;
  idx += bb_idx * BASIC_BLK / LONG_BIT;
  bb_idx = pos / LONG_BIT;
  while (idx < bb_idx)
  {
    rank += LONG_BIT - __builtin_popcountll(_bv[idx]);
    idx++;
  }

  idx = pos % LONG_BIT;
  if (idx > 0)
  {
    uint64_t mask = (1ULL << idx) - 1;
    rank += idx - __builtin_popcountll(_bv[bb_idx] & mask);
  }
  return rank;
}

// return zero based position
uint64_t
BitVector::select0(uint64_t rank)
{
  if (0 == rank || rank > (_size - _ones))
  {
    return ULONG_LONG_MAX;
  }

  // pos is zero based
  uint32_t pos = _sb0[rank / (SELECT_BLK + 1)];

  // upper block idx, zero based
  uint32_t idx = pos / UPPER_BLK;

  // last upper blocks idx, zero based
  uint32_t last_ub = (_size - 1) / UPPER_BLK;

  while (idx < last_ub &&
        ((idx + 1) * UPPER_BLK - _ub[2 * (idx + 1)]) < rank)
  {
    ++idx;
  }

  // remaing
  rank -= idx * UPPER_BLK - _ub[2 * idx];

  // basic blocks
  uint32_t bb_idx = 0;
  const uint32_t last_bb = std::min(3U, static_cast<uint32_t>((_size % UPPER_BLK - 1) / BASIC_BLK));
  while (bb_idx < last_bb)
  {
    // rank of the basic block
    uint32_t temp = BASIC_BLK - (MASK_10BIT & (_ub[2 * idx + 1] >> (bb_idx * TEN_BITS)));
    if (rank <= temp)
    {
      break;
    }
    rank -= temp;
    ++bb_idx;
  }

  pos = idx * UPPER_BLK + bb_idx * BASIC_BLK;
  assert(rank > 0);

  // this is the basic block, basic block has 8 64bits
  uint32_t bv_idx = (pos / LONG_BIT);
  uint32_t last_bv = (_size - 1) / LONG_BIT;
  for (uint32_t r = 0, cnt = std::min(last_bv, 8U); r < cnt; ++r, ++bv_idx)
  {
    uint32_t temp = LONG_BIT - __builtin_popcountll(_bv[bv_idx]);
    if (rank <= temp)
    {
      break;
    }
    rank -= temp;
    pos += LONG_BIT;
  }

  // bv_idx contains answer
  uint64_t mask = 1ULL;
  while (mask > 0)
  {
    if (0 == (_bv[bv_idx] & mask) && 0 == --rank)
    {
      break;
    }

    ++pos;
    mask <<= 1;
  }

  return pos;
}

void
BitVector::updatePageSegments()
{
  uint32_t max = 0;
  for (std::vector<const Segment *>::const_iterator ci = _grp.begin();
       ci != _grp.end(); ++ci)
  {
    if ((*ci)->pages > max)
    {
      max = (*ci)->pages;
    }
  }
  _working_pages = max;

  uint8_t pages = 0;
  for (std::vector<const Segment *>::const_iterator ci = _grp.begin();
       ci != _grp.end(); ++ci)
  {
    pages += max / (*ci)->pages;
  }
  _pages = pages;
}
