#include <megatree/compress.h>
#include <megatree/tree_common.h>
#include <cstdio>

namespace megatree
{

  const static unsigned NEW_NODE_SIZE = 3 + 1 + 4;

  void bitcpy(uint64_t& dest, uint64_t src, unsigned dest_pos, unsigned src_pos, unsigned num)
  {
    uint64_t src_mask = ((1 << (num-1)) * 2 - 1) << src_pos;
    uint64_t src_bits = (src & src_mask) >> src_pos;
    uint64_t dest_bits = src_bits << dest_pos;
    dest |= dest_bits;
  }

  void compress(const ByteVec& data, ByteVec& res)
  {
    uint64_t num_nodes = (data.size()-1)/NODE_SIZE;
    //printf("Compressing %d nodes, data size %d\n", num_nodes, data.size());

    res.resize(num_nodes*NEW_NODE_SIZE+9);  // max possible size

    unsigned res_offset = 0;
    unsigned data_offset = 0;
    uint32_t last_short_id = 0;

    // write number of nodes in 8 bytes
    memcpy(&res[res_offset], &num_nodes, 8);
    res_offset += 8;

    // children of file
    memcpy(&res[res_offset], &data[data_offset], 1);
    res_offset += 1;
    data_offset += 1;

    while (data_offset < data.size())
    {
      uint64_t tmp = 0; 
      unsigned bit_offset = 0;

      // three bits per point component
      for (unsigned i=0; i<3; i++)
      {      
        Point pnt = 0;
        memcpy(&pnt, &data[data_offset], POINT_SIZE / 3);
        bitcpy(tmp, pnt, bit_offset, 13, 3);
        bit_offset += 3;
        data_offset += POINT_SIZE / 3;
      }
    
      // four bits per color component
      for (unsigned i=0; i<3; i++)
      {      
        Color col = 0;
        memcpy(&col, &data[data_offset], COLOR_SIZE / 3);
        bitcpy(tmp, col, bit_offset, 4, 4);
        bit_offset += 4;
        data_offset += COLOR_SIZE / 3;
      }

      // pre-read short id
      uint32_t short_id;
      memcpy(&short_id, &data[data_offset + COUNT_SIZE + CHILDREN_SIZE], SHORT_ID_SIZE);

      // three bits for short id diff byte length
      unsigned short_id_bytes = 4;
      uint32_t short_id_diff = short_id - last_short_id;
      last_short_id = short_id;
      if (short_id_diff < 256)
        short_id_bytes = 1;
      else if (short_id_diff < 65536)
        short_id_bytes = 2;
      else if (short_id_diff < 16777216)
        short_id_bytes = 3;
      bitcpy(tmp, short_id_bytes, bit_offset, 0, 3);

      memcpy(&res[res_offset], &tmp, 3);  // copy point, color and short_id_bytes into res
      res_offset += 3;

      // skip count
      data_offset += COUNT_SIZE;

      // children of node
      memcpy(&res[res_offset], &data[data_offset], CHILDREN_SIZE);
      res_offset += CHILDREN_SIZE;
      data_offset += CHILDREN_SIZE;
    
      // short id diff
      memcpy(&res[res_offset], &short_id_diff, short_id_bytes);
      res_offset += short_id_bytes;
      data_offset += SHORT_ID_SIZE;
    }    
    res.resize(res_offset);

    printf("Compressed %zu nodes, compression ratio %f\n", num_nodes, (float)res.size() / (float)data.size());
  }


  void extract(const ByteVec& data, ByteVec& res)
  {
    unsigned res_offset = 0;
    unsigned data_offset = 0;
    uint32_t last_short_id = 0;

    // read number of nodes
    uint64_t num_nodes;
    memcpy(&num_nodes, &data[data_offset], 8);
    data_offset += 8;

    // resize result
    res.resize(num_nodes*NODE_SIZE+1);
    printf("Extracting %zu nodes, compression ratio %f\n", num_nodes, (float)data.size() / (float)res.size());

    // first byte is children
    memcpy(&res[res_offset], &data[data_offset], 1);
    res_offset += 1;
    data_offset += 1;

    while (data_offset < data.size())
    {
      // read point and color data as first three bytes
      uint64_t tmp;
      memcpy(&tmp, &data[data_offset], 3);      
      data_offset += 3;
      unsigned bit_offset = 0;

      // three bits per point component
      for (unsigned i=0; i<3; i++)
      {
        uint64_t pnt = 0;
        bitcpy(pnt, tmp, 13, bit_offset, 3);
        bit_offset += 3;
        memcpy(&res[res_offset], &pnt, POINT_SIZE / 3);
        res_offset += POINT_SIZE / 3;
      }

      // four bits per color component
      for (unsigned i=0; i<3; i++)
      {
        uint64_t col = 0;
        bitcpy(col, tmp, 4, bit_offset, 4);
        bit_offset += 4;
        memcpy(&res[res_offset], &col, COLOR_SIZE / 3);
        res_offset += COLOR_SIZE / 3;
      }

      // three bits for short id diff byte size
      uint64_t short_id_bytes = 0;
      bitcpy(short_id_bytes, tmp, 0, bit_offset, 3);
      assert(short_id_bytes > 0);
      assert(short_id_bytes < 5);

      // padding for count
      res_offset += COUNT_SIZE;

      // children of node
      memcpy(&res[res_offset], &data[data_offset], CHILDREN_SIZE);
      //printf("Children %d\n", (int)(res[res_offset]));
      res_offset += CHILDREN_SIZE;
      data_offset += CHILDREN_SIZE;
    
      // short id 
      uint32_t short_id_diff = 0, short_id;
      memcpy(&short_id_diff, &data[data_offset], short_id_bytes);
      short_id = last_short_id + short_id_diff;
      last_short_id = short_id;
      memcpy(&res[res_offset], &short_id, SHORT_ID_SIZE);      
      res_offset += SHORT_ID_SIZE;
      data_offset += short_id_bytes;
    }    
  }


}//namespace
