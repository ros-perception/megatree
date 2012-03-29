#ifndef MEGATREE_NODE_GEOMETRY_H
#define MEGATREE_NODE_GEOMETRY_H

#include <megatree/tree_common.h>
#include <assert.h>
#include <vector>


namespace megatree
{

class NodeGeometry
{
public:
  NodeGeometry()
  {}

  NodeGeometry(const std::vector<double>& tree_center, double tree_size)
    : level(1)
  {
    assert(tree_center.size() == 3);
    for (int i = 0; i < 3; ++i) {
      lo[i] = tree_center[i] - tree_size / 2.0;
      hi[i] = tree_center[i] + tree_size / 2.0;
    }
  }

  NodeGeometry(const double tree_center[3], double tree_size)
    : level(1)
  {
    for (int i = 0; i < 3; ++i) {
      lo[i] = tree_center[i] - tree_size / 2.0;
      hi[i] = tree_center[i] + tree_size / 2.0;
    }
  }
  NodeGeometry(int _level, const double _lo[3], const double _hi[3])
    : level(_level)
  {
    lo[0] = _lo[0];  lo[1] = _lo[1];  lo[2] = _lo[2];
    hi[0] = _hi[0];  hi[1] = _hi[1];  hi[2] = _hi[2];
  }

  NodeGeometry(const NodeGeometry& ng)
  {
    lo[0] = ng.lo[0];  lo[1] = ng.lo[1];  lo[2] = ng.lo[2];
    hi[0] = ng.hi[0];  hi[1] = ng.hi[1];  hi[2] = ng.hi[2];
    level = ng.level;
  }


  // Fills the child NodeGeometry of child number 'to_child'
  NodeGeometry getChild(uint8_t to_child) const
  {
    NodeGeometry res;
    res.level = level + 1;
    
    double mid[] = {(lo[0] + hi[0]) / 2.0,
                    (lo[1] + hi[1]) / 2.0,
                    (lo[2] + hi[2]) / 2.0 };

    res.lo[0] = (to_child & (1 << X_BIT)) ? mid[0] : lo[0];
    res.hi[0] = (to_child & (1 << X_BIT)) ? hi[0]  : mid[0];
    res.lo[1] = (to_child & (1 << Y_BIT)) ? mid[1] : lo[1];
    res.hi[1] = (to_child & (1 << Y_BIT)) ? hi[1]  : mid[1];
    res.lo[2] = (to_child & (1 << Z_BIT)) ? mid[2] : lo[2];
    res.hi[2] = (to_child & (1 << Z_BIT)) ? hi[2]  : mid[2];

    return res;
  }


  // Fills the parent NodeGeometry, given that this NodeGeometry is
  // child 'from_child' of the parent.  This method may give
  // "incorrect" results because of floating point rounding error (in
  // other words,  `ng.getChild(0).getParent(0) == ng` may be false).
  NodeGeometry getParent(uint8_t from_child) const
  {
    NodeGeometry res;
    assert(level > 0);
    
    res.level = level - 1;

    double s[] = {hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]};
    
    res.lo[0] = (from_child & (1 << X_BIT)) ? lo[0] - s[0] : lo[0];
    res.hi[0] = (from_child & (1 << X_BIT)) ? hi[0]        : hi[0] + s[0];
    res.lo[1] = (from_child & (1 << Y_BIT)) ? lo[1] - s[1] : lo[1];
    res.hi[1] = (from_child & (1 << Y_BIT)) ? hi[1]        : hi[1] + s[1];
    res.lo[2] = (from_child & (1 << Z_BIT)) ? lo[2] - s[2] : lo[2];
    res.hi[2] = (from_child & (1 << Z_BIT)) ? hi[2]        : hi[2] + s[2];

    return res;
  }


  // Returns which child of this NodeGeometry would contain pnt.
  uint8_t whichChild(const double pnt[3]) const
  {
    double center[] = {(lo[0] + hi[0]) / 2.0,
                       (lo[1] + hi[1]) / 2.0,
                       (lo[2] + hi[2]) / 2.0 };
    return  (pnt[0] >= center[0]) << X_BIT | 
            (pnt[1] >= center[1]) << Y_BIT | 
            (pnt[2] >= center[2]) << Z_BIT;
  }


  // Returns the edge length of the node cell
  double getSize() const
  {
    return hi[0] - lo[0];
  }
  double getCenter(int i) const
  {
    return (hi[i] + lo[i]) / 2.0;
  }

  double getLo(int i) const { return lo[i]; }
  double getHi(int i) const { return hi[i]; }

  // Checks if a point is inside the bounds of the node geometry
  bool contains(const double pt[3]) const { return contains(pt[0], pt[1], pt[2]); }
  bool contains(const std::vector<double> &pt) const { return contains(pt[0], pt[1], pt[2]); }
  bool contains(double x, double y, double z) const
  {
    return
      lo[0] <= x && x < hi[0] &&
      lo[1] <= y && y < hi[1] &&
      lo[2] <= z && z < hi[2];
  }

  NodeGeometry& operator=(const NodeGeometry& ng)
  {
    level = ng.level;
    lo[0] = ng.lo[0];  lo[1] = ng.lo[1];  lo[2] = ng.lo[2];
    hi[0] = ng.hi[0];  hi[1] = ng.hi[1];  hi[2] = ng.hi[2];
    return *this;
  }

  unsigned int getLevel() const { return level; }

private:
  double lo[3], hi[3];  // Minimum point and maximum point of this cell
  unsigned int level;
};

}

#endif
