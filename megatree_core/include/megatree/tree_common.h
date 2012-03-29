#ifndef MEGATREE_TREE_COMMON_H
#define MEGATREE_TREE_COMMON_H

#include <vector>
#include <boost/cstdint.hpp>

namespace megatree{

static const int X_BIT = 2;
static const int Y_BIT = 1;
static const int Z_BIT = 0;

typedef uint32_t ShortId;
typedef uint64_t Count;
typedef uint8_t Color;
typedef uint16_t Point;
typedef std::vector<uint8_t> ByteVec;

//the size of each node in bytes
const static int POINT_SIZE = 3*sizeof(Point);
const static int COLOR_SIZE = 3*sizeof(Color);
const static int COUNT_SIZE = sizeof(Count);
const static int CHILDREN_SIZE = sizeof(uint8_t);
const static int SHORT_ID_SIZE = sizeof(ShortId);
const static int NODE_SIZE = POINT_SIZE + COLOR_SIZE + COUNT_SIZE + CHILDREN_SIZE + SHORT_ID_SIZE;
}


#endif
