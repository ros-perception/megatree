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

}

#endif
