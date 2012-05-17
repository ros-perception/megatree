#ifndef MEGATREE_LONG_ID_H
#define MEGATREE_LONG_ID_H

#include <cstdio>
#include <boost/cstdint.hpp>
#include <assert.h>
#include <iostream>
#include <math.h>

namespace megatree
{

template <int num>
class LongId
{
public:
  LongId<num>(uint64_t value=0)
  {
    // initialize the value of the node
    for (int i=0; i<num-1; i++)
      ints[i] = 0;
    ints[num-1] = value;

    // compute the level of this node
    id_level = 0;
    while (value != 0)
    {
      value >>= 3;
      id_level++;
    }
  }


  bool operator < (const LongId<num>& value) const
  {
    for (int i=0; i<num; i++)
    {
      if (ints[i] < value.ints[i])
        return true;
      else if (ints[i] > value.ints[i])
        return false;
    }

    // equal
    return false;
  }

  bool operator == (const LongId<num>& value) const
  {
    if (id_level != value.id_level)
      return false;

    for (int i=0; i<num; i++)
    {
      if (ints[i] != value.ints[i])
        return false;
    }

    // equal
    return true;
  }

  LongId<num> &operator=(const LongId<num> &o)
  {
    id_level = o.id_level;
    for (int i = 0; i < num; ++i)
      ints[i] = o.ints[i];
    return *this;
  }


  bool operator != (const LongId<num>& value) const
  {
    return !(*this == value);
  }



  unsigned int level() const
  {
    return id_level;
  }


  bool isRoot() const
  {
    return (id_level == 1);
  }
  bool isRootFile() const
  {
    return id_level == 0;
  }

  bool isValid() const
  {
    return (id_level != 0);
  }


  uint8_t getChildNr() const
  {
    return ints[num-1] & 7;
  }

  int getBit(int bit) const
  {
    assert(bit <= 63);
    return (ints[num-1] >> bit) & 1;
  }

  int getBits(uint8_t nr_bits) const
  {
    return (ints[num-1] & ((1 << nr_bits)-1));
  }


  std::string toString() const
  {
    LongId<num> id = *this;
    std::string s;
    s.resize(level()+1);
    int i = s.size();
    while (id.isValid())
    {
      s[--i] = (char)((int)'0'+(id.ints[num-1] & 7));
      id = id.getParent();
    }
    s[--i] = '0';
    return s;
  }

  void toPath(unsigned folder_depth, std::string& path, std::string& file) const
  {
    LongId<num> id = *this;
    int file_size = level()%folder_depth;
    int path_size = (level() - file_size);

    // get filename
    file.resize(file_size+1);
    for (int i=file_size; i>0; i--)
    {
      file[i] = (char)((int)'0'+(id.ints[num-1] & 7));
      id = id.getParent();
    }
    file[0] = 'f';
    
    // get path
    path.resize(path_size + path_size/folder_depth);
    int count = path.size();
    for (int i=0; i<path_size; i++)
    {
      if (i%folder_depth == 0)
        path[--count] = '/';
      path[--count] = (char)((int)'0'+(id.ints[num-1] & 7));
      id = id.getParent();
    }

    //printf("level %d, file_size %d, path_size %d\n", level(), file_size, path_size);    
    //printf("Id %s gets path %s and file %s\n", toString().c_str(), path.c_str(), file.c_str());
  }


  unsigned getMaxDepth() const
  {
    unsigned bits = num*64-1;
    return (bits - (bits % 3)) / 3;
  }

  // Returns the id of the parent, moving up the tree by "generations" number of steps.
  LongId<num> getParent(unsigned int generations=1) const
  {
    // The parent doesn't exist, so it returns the invalid id.
    if (generations >= id_level)
      return LongId<num>();

    assert(3*generations < 64);
    unsigned int shift = 3*generations;

    LongId<num> parent;

    for (int i = num - 1; i > 0; i--)
    {
      parent.ints[i] = (ints[i] >> shift) + ((ints[i-1] & ((1 << shift)-1)) << (64-shift));
    }
    parent.ints[0] = ints[0] >> shift;

    // Computes the parent's level.
    parent.id_level = id_level - generations;

    return parent;
  }



  LongId<num> getChild(int child_nr) const
  {
    LongId<num> child;

    uint64_t oct = 7;
    oct <<= 61;
    for (int i=0; i<num-1; i++)
    {
      child.ints[i] = (ints[i] << 3) + ((ints[i+1] & oct) >> 61);
    }
    child.ints[num-1] = (ints[num-1] << 3) + child_nr;
    child.id_level = id_level+1;
    
    assert(id_level < num*64/3);

    return child;
  }


  int size() const
  {
    return num;
  }

  uint64_t ints[num];
  unsigned int id_level;

};


template <int num>
std::ostream& operator << (std::ostream& os, const LongId<num>& value) 
{
  for (int i=0; i<(int)value.size(); i++)
    os << value.toString();
  return os;
}


typedef LongId<2> IdType;







} // namespace

#include <tr1/unordered_map>
namespace std {
  namespace tr1 {
    template <>
      struct hash<megatree::LongId<2> >
      {
        std::size_t operator()(const megatree::LongId<2>& li) const {
          std::tr1::hash<uint64_t> hasher;
          return hasher(li.ints[0] ^ li.ints[1]);
        }
      };
  }
}

#endif
