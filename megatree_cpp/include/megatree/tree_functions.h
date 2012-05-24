#ifndef MEGATREE_TREE_FUNCTIONS_H
#define MEGATREE_TREE_FUNCTIONS_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "megatree/node.h"
#include "megatree/megatree.h"


namespace megatree
{
void addPoint(MegaTree &tree, const std::vector<double> &pt, const std::vector<double>& color = std::vector<double>(3, 0));


void queryRange(MegaTree &tree, const std::vector<double>& lo, const std::vector<double>&hi,
                double resolution, std::vector<double> &results, std::vector<double> &colors);

void numChildren(MegaTree& tree, NodeHandle* node, unsigned count_cutoff, unsigned& num_children, unsigned& count);
void dumpTimers();

void queryRangeIntersecting(MegaTree &tree, NodeHandle& node, 
                            const double* range_mid, const double* range_size,
                            std::vector<double> &results, std::vector<double> &colors);

bool nodeInsideRange(const NodeGeometry& node_geom, const double* range_mid, const double* range_size);
bool nodeOutsideRange(const NodeGeometry& node_geom, const double* range_mid, const double* range_size);


void rangeQueryLoop(MegaTree& tree, std::vector<double> lo, std::vector<double> hi,
                    double resolution, std::vector<double>& results, std::vector<double>& colors);




class NodeCache
{
public:
  NodeCache(NodeHandle* nh_p, MegaTree& tree)
    : nh(nh_p),
      count(0)
  {
    for (unsigned i=0; i<3; i++){
      sum_point[i] = 0;
      sum_color[i] = 0;
    }

    for (unsigned int i = 0; i < 8; i++)
    {
      if (nh->hasChild(i))
      {
        NodeHandle child_nh;
        tree.getChildNode(*nh, i, child_nh);
        const Count& child_cnt = child_nh.getNode()->count;
        const Point* child_pnt = child_nh.getNode()->point;
        const Color* child_col = child_nh.getNode()->color;
        
        uint64_t child_offset[3];
        nh->getNode()->getChildBitOffset(i, child_offset);

        sum_point[0] += child_cnt * (child_pnt[0] + child_offset[0]);
        sum_point[1] += child_cnt * (child_pnt[1] + child_offset[1]);
        sum_point[2] += child_cnt * (child_pnt[2] + child_offset[2]);

        sum_color[0] += child_cnt * child_col[0];
        sum_color[1] += child_cnt * child_col[1];
        sum_color[2] += child_cnt * child_col[2];

        // TODO: sum will overflow at some point.
        count += child_cnt;
        tree.releaseNode(child_nh);
      }
    }
  }


  void addPoint(NodeCache& child_cache)
  {
    uint64_t child_offset[3];
    nh->modified = true;
    nh->getNode()->getChildBitOffset(child_cache.node_id, child_offset);

    Point* child_pnt = child_cache.nh->getNode()->point;
    Color* child_col = child_cache.nh->getNode()->color;
    Point* pnt = nh->getNode()->point;
    Color* col = nh->getNode()->color;
    uint64_t&  cnt = nh->getNode()->count;

    sum_point[0] += child_pnt[0] + child_offset[0];
    sum_point[1] += child_pnt[1] + child_offset[1];
    sum_point[2] += child_pnt[2] + child_offset[2];
    
    sum_color[0] += child_col[0];
    sum_color[1] += child_col[1];
    sum_color[2] += child_col[2];

    count += 1;

    pnt[0] = (sum_point[0] / count) >> 1;
    pnt[1] = (sum_point[1] / count) >> 1;
    pnt[2] = (sum_point[2] / count) >> 1;

    col[0] = sum_color[0] / count;
    col[1] = sum_color[1] / count;
    col[2] = sum_color[2] / count;

    cnt++;
  }

  NodeHandle* nh;
  uint64_t sum_point[3];
  uint64_t sum_color[3];
  uint64_t count;
  uint8_t node_id;
};


class TreeFastCache
{
public:
  TreeFastCache(MegaTree &tree_p):
    tree(tree_p)
  {
    NodeHandle* root = tree.getRoot();
    nodes.push_back(NodeCache(root, tree));
  }

  ~TreeFastCache()
  {
    while (!nodes.empty()){
      NodeHandle* n = nodes.back().nh;
      tree.releaseNode(*n);
      delete n;
      nodes.pop_back();
    }
  }

  void addPoint(std::vector<double> &pt, const std::vector<double>& color = std::vector<double>(3, 0));


private:
  void addPointRecursive(const double pt[3], const double color[3], double point_accuracy);

  std::list<NodeCache> nodes;
  MegaTree& tree;
};



}

#endif
