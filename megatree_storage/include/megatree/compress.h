#ifndef MEGATREE_COMPRESS_H
#define MEGATREE_COMPRESS_H

#include <megatree/storage.h>

namespace megatree
{

  void bitcpy(uint64_t& dest, uint64_t src, unsigned dest_pos, unsigned src_pos, unsigned num);

  void compress(const ByteVec& data, ByteVec& res);
  void extract(const ByteVec& data, ByteVec& res);


}// namespace

#endif
