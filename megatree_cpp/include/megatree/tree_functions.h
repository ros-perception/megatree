#ifndef MEGATREE_TREE_FUNCTIONS_H
#define MEGATREE_TREE_FUNCTIONS_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "megatree/node.h"
#include "megatree/megatree.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>

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
  NodeCache(NodeHandle* nh_p)
    : nh(nh_p)
  {
    // copy value of original point
    orig_cnt = nh->getCount();
    Point* pnt = nh->getNode()->point;
    Color* col = nh->getNode()->color;
    for (unsigned i=0; i<3; i++){
      orig_pnt[i] = pnt[i];
      orig_col[i] = col[i];
    }
  }

  NodeCache(const NodeCache& nc)
  {
    nh = nc.nh;
    orig_cnt= nc.orig_cnt;
    
    for (unsigned i=0; i<3; i++){
      orig_pnt[i] = nc.orig_pnt[i];
      orig_col[i] = nc.orig_col[i];
    }
  }

  NodeCache& operator =(const NodeCache& nc)
  {
    nh = nc.nh;
    orig_cnt= nc.orig_cnt;
    
    for (unsigned i=0; i<3; i++){
      orig_pnt[i] = nc.orig_pnt[i];
      orig_col[i] = nc.orig_col[i];
    }
    return *this;
  }


  ~NodeCache()
  {
  }


  // add sum info to NodeHandle
  void release(MegaTree& tree)
  {
    tree.releaseNode(*nh);
    delete nh;
    nh = NULL;
  }


  void mergeChild(const NodeCache& nc)
  {
    assert(nc.nh);
    assert(nh);

    // child offset
    uint64_t child_offset[3];
    nh->getNode()->getChildBitOffset(nc.nh->getId().getChildNr(), child_offset);

    // compute difference between original child node and the  current child node
    uint64_t child_cnt_curr = nc.nh->getCount();
    uint64_t diff_sum_pnt[3], diff_sum_col[3];
    uint64_t diff_cnt = child_cnt_curr-nc.orig_cnt;
    for (unsigned i=0; i<3; i++){
      diff_sum_pnt[i] = nc.nh->getNode()->point[i]*child_cnt_curr - nc.orig_pnt[i]*nc.orig_cnt + child_offset[i]*diff_cnt;
      diff_sum_col[i] = nc.nh->getNode()->color[i]*child_cnt_curr - nc.orig_col[i]*nc.orig_cnt;
    }    

    // update point of this node
    Point* pnt = nh->getNode()->point;
    Color* col = nh->getNode()->color;
    uint64_t& cnt = nh->getNode()->count;
    for (unsigned i=0; i<3; i++){
      pnt[i] = (pnt[i]*cnt + diff_sum_pnt[i]) / (cnt + diff_cnt);
      col[i] = (col[i]*cnt + diff_sum_col[i]) / (cnt + diff_cnt);
    }
    cnt += diff_cnt;
  }


  NodeHandle* nh;
  uint64_t orig_pnt[3];
  uint64_t orig_col[3];
  uint64_t orig_cnt;
};




class TreeFastCache
{
public:
  TreeFastCache(MegaTree &tree_p):
    tree(tree_p)
  {
    push(NodeCache(tree.getRoot()));
  }

  ~TreeFastCache()
  {
    while (!nodes.empty())
      pop();
  }

  void clear()
  {
    while (!nodes.empty())
      pop();

    push(NodeCache(tree.getRoot()));
  }

  void addPoint(std::vector<double> &pt, const std::vector<double>& color = std::vector<double>(3, 0));


private:
  NodeCache& top()
  {
    assert(!nodes.empty());
    return nodes.top();
  }

  void push(NodeCache nc)
  {
    nodes.push(nc);
  }

  void pop()
  {
    assert(!nodes.empty());

    NodeCache child = nodes.top();
    nodes.pop();
    if (!nodes.empty())
      nodes.top().mergeChild(child);

    child.release(tree);  // after this, the child is written and destroyed
  }

  void addPointRecursive(const double pt[3], const double color[3], double point_accuracy);

  std::stack<NodeCache> nodes;
  MegaTree& tree;
};



}

#endif
