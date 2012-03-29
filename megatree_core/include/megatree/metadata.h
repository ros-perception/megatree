#ifndef MEGATREE_METADATA_H
#define MEGATREE_METADATA_H

#include <megatree/tree_common.h>

namespace megatree
{

class MetaData
{
public:
  MetaData() {};
  MetaData(unsigned _version, unsigned _subtree_width, unsigned _subfolder_depth,
           double _min_cell_size, double _root_size, const std::vector<double>& _root_center)
    : version(_version), subtree_width(_subtree_width), subfolder_depth(_subfolder_depth),
      min_cell_size(_min_cell_size), root_size(_root_size), root_center(_root_center) 
  {}

  void deserialize(const ByteVec& data);
  void serialize(ByteVec& data);


  unsigned version, subtree_width, subfolder_depth;
  double min_cell_size, root_size;
  std::vector<double> root_center;

}; // class
} // namespace

#endif
