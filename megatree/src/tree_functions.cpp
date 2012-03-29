#include "megatree/tree_functions.h"
#include <boost/bind.hpp>
#include <megatree/common.h>  // For the timers


namespace megatree
{
//PausingTimer overall_add_timer, getnode_timer, create_leaf_timer, create_node_timer;
void dumpTimers()
{
  /*
    printf("Overall:      %8.3f\n", overall_add_timer.getElapsed());
    printf("getNode:      %8.3f\n", getnode_timer.getElapsed());
    printf("create leaf:  %8.3f\n", create_leaf_timer.getElapsed());
    printf("create node:  %8.3f\n", create_node_timer.getElapsed());
    printf("unknown:      %8.3f\n", overall_add_timer.getElapsed() -
    (create_node_timer.getElapsed() + getnode_timer.getElapsed() + create_leaf_timer.getElapsed()));
    overall_add_timer.reset();
    getnode_timer.reset();
    create_leaf_timer.reset();
    create_node_timer.reset();
  */
}

void addPointRecursive(MegaTree& tree, NodeHandle& node, 
                       const double* pt, const double* color,
                       double point_accuracy)
{
  //printf("Adding point %f %f %f to node %s\n", pt[0], pt[1], pt[2], node.toString().c_str());
  double node_cell_size = node.getNodeGeometry().getSize();

  // adds point to empty node
  if (node.isEmpty() && point_accuracy >= node_cell_size / BITS_D)
  {
    node.setPoint(pt, color);
    return;
  }
    
  // reached maximum resultion of the tree
  if (node_cell_size < tree.getMinCellSize())
  {
    node.addPoint(pt, color);
    return;
  }


  // this node is a leaf, and we need to copy the original point one level down
  if (!node.isEmpty() && node.isLeaf())
  {
    // Determines which child the point should be added to.
    uint8_t original_child = node.getChildForNodePoint();

    //create_leaf_timer.start();
    NodeHandle new_leaf;
    tree.createChildNode(node, original_child, new_leaf);
    //create_leaf_timer.pause();

    node.getNode()->copyToChildNode(original_child, new_leaf.getNode());
    tree.releaseNode(new_leaf);
  }

  // Gets the child node to recurse on.
  uint8_t new_child = node.getNodeGeometry().whichChild(pt);
  NodeHandle new_child_node;
  if (node.hasChild(new_child)) {
    //getnode_timer.start();
    tree.getChildNode(node, new_child, new_child_node);
    new_child_node.waitUntilLoaded();
    //getnode_timer.pause();
  }
  else {
    //create_node_timer.start();
    tree.createChildNode(node, new_child, new_child_node);
    //create_node_timer.pause();
  }

  // recursion to add point to child
  addPointRecursive(tree, new_child_node, pt, color, point_accuracy);

  tree.releaseNode(new_child_node);

  // Re-computes the summary point.
  MegaTree::ChildIterator it(tree, node, new_child_node.getNodeFile());
  node.copyFromChildNodes(it.getAllChildren());
}




void addPoint(MegaTree& tree, const std::vector<double>& pt, const std::vector<double>& col)
{
  // check tree bounds
  if (!tree.getRootGeometry().contains(pt))
  {
    fprintf(stderr, "Point (%lf, %lf, %lf) is out of tree bounds (%lf, %lf, %lf)--(%lf, %lf, %lf)\n",
            pt[0], pt[1], pt[2],
            tree.getRootGeometry().getLo(0), tree.getRootGeometry().getLo(1), tree.getRootGeometry().getLo(2),
            tree.getRootGeometry().getHi(0), tree.getRootGeometry().getHi(1), tree.getRootGeometry().getHi(2));
    return;
  }

  NodeHandle root;
  tree.getRoot(root);
  //overall_add_timer.start();
  addPointRecursive(tree, root, &pt[0], &col[0], tree.getMinCellSize());
  //overall_add_timer.pause();

  tree.releaseNode(root);
}
  






void queryRangeIntersecting(MegaTree &tree, NodeHandle& node, 
                            const double* range_mid, const double* range_size,
                            std::vector<double> &results, std::vector<double> &colors)
{
  /*
    printf("Query range node for node %s with frame offset %f %f %f, range mid %f %f %f and size %f %f %f\n",
    node.getId().toString().c_str(), frame_offset[0], frame_offset[1], frame_offset[2],
    range_mid[0], range_mid[1], range_mid[2], range_size[0], range_size[1], range_size[2]);
  */

  assert(!node.isEmpty());
  double point[3];
  node.getPoint(point);

  if (point[0] >= range_mid[0] - range_size[0]/2 &&
      point[0] <  range_mid[0] + range_size[0]/2 &&
      point[1] >= range_mid[1] - range_size[1]/2 &&
      point[1] <  range_mid[1] + range_size[1]/2 &&
      point[2] >= range_mid[2] - range_size[2]/2 &&
      point[2] <  range_mid[2] + range_size[2]/2)
  {
    results.push_back(point[0]);
    results.push_back(point[1]);
    results.push_back(point[2]);

    double color[3];
    node.getColor(color);
    colors.push_back(color[0]);
    colors.push_back(color[1]);
    colors.push_back(color[2]);
  }
}


// frame_offset
void getAllPointsRecursive(MegaTree &tree, NodeHandle& node, double resolution, std::vector<double> &results, std::vector<double> &colors)
{
  //printf("Get all points recursive for node %s with frame offset %f %f %f\n",
  //       node.getId().toString().c_str(), frame_offset[0], frame_offset[1], frame_offset[2]);

  assert(!node.isEmpty());

  // we hit a leaf
  if (node.isLeaf() || node.getNodeGeometry().getSize() <= resolution) 
  {
    double point[3];
    node.getPoint(point);
    results.push_back(point[0]);
    results.push_back(point[1]);
    results.push_back(point[2]);

    double color[3];
    node.getColor(color);
    colors.push_back(color[0]);
    colors.push_back(color[1]);
    colors.push_back(color[2]);
  }

  // need to descend further
  else 
  {
    for (MegaTree::ChildIterator it(tree, node); !it.finished(); it.next())
    {
      getAllPointsRecursive(tree, it.getChildNode(), resolution, results, colors);
    }
  }
}


bool nodeOutsideRange(const NodeGeometry& node_geom, const double* range_mid, const double* range_size)
{
  return
    node_geom.getHi(0) <= (range_mid[0] - range_size[0]/2) || node_geom.getLo(0) >= (range_mid[0] + range_size[0]/2) ||
    node_geom.getHi(1) <= (range_mid[1] - range_size[1]/2) || node_geom.getLo(1) >= (range_mid[1] + range_size[1]/2) ||
    node_geom.getHi(2) <= (range_mid[2] - range_size[2]/2) || node_geom.getLo(2) >= (range_mid[2] + range_size[2]/2);
}

bool nodeInsideRange(const NodeGeometry& node_geom, const double* range_mid, const double* range_size)
{
  return
    (range_mid[0] - range_size[0]/2) <= node_geom.getLo(0) && node_geom.getHi(0) <= (range_mid[0] + range_size[0]/2) &&
    (range_mid[1] - range_size[1]/2) <= node_geom.getLo(1) && node_geom.getHi(1) <= (range_mid[1] + range_size[1]/2) &&
    (range_mid[2] - range_size[2]/2) <= node_geom.getLo(2) && node_geom.getHi(2) <= (range_mid[2] + range_size[2]/2);
}


void queryRangeRecursive(MegaTree &tree, NodeHandle& node, 
                         const double* range_mid, const double* range_size,
                         double resolution, std::vector<double> &results, std::vector<double> &colors)
{
  /*
    printf("Query recursive for node %s with %f points frame offset %f %f %f, range mid %f %f %f and size %f %f %f\n",
    node.getId().toString().c_str(), node.getCount(), frame_offset[0], frame_offset[1], frame_offset[2],
    range_mid[0], range_mid[1], range_mid[2], range_size[0], range_size[1], range_size[2]); 
  */

  // If the query range is outside of the node.
  if (nodeOutsideRange(node.getNodeGeometry(), range_mid, range_size))
  {
    // Completely outside.  Stop
  }
  else if (nodeInsideRange(node.getNodeGeometry(), range_mid, range_size)) 
  {
    // Completely inside
    getAllPointsRecursive(tree, node, resolution, results, colors);
  }

  // Partially intersecting
  else 
  { 
    // node is leaf
    if (node.isLeaf() || node.getNodeGeometry().getSize() <= resolution)
    {  
      queryRangeIntersecting(tree, node, range_mid, range_size, results, colors);
    }
    // node is not a leaf
    else   
    {
      for (MegaTree::ChildIterator it(tree, node); !it.finished(); it.next())
      {
        // recurse to child
        queryRangeRecursive(tree, it.getChildNode(), range_mid, range_size, resolution, results, colors);
      }
    }
  }
}



void queryRange(MegaTree &tree, const std::vector<double>& lo, const std::vector<double>&hi,
                double resolution, std::vector<double> &results, std::vector<double> &colors)
{
  if (results.size() > 0)
    fprintf(stderr, "Warning: called queryRange with non-empty results\n");

  NodeHandle root;
  tree.getRoot(root);
  double range_mid[3];
  range_mid[0] = (hi[0] + lo[0])/2;
  range_mid[1] = (hi[1] + lo[1])/2;
  range_mid[2] = (hi[2] + lo[2])/2;
  double range_size[3];
  range_size[0] = hi[0] - lo[0];
  range_size[1] = hi[1] - lo[1];
  range_size[2] = hi[2] - lo[2];

  queryRangeRecursive(tree, root, range_mid, range_size, resolution, results, colors);
  tree.releaseNode(root);
}





void numChildren(MegaTree& tree, NodeHandle& node, unsigned count_cutoff, unsigned& num_children, unsigned& count)
{
  printf("Node %s\n", node.toString().c_str());
  if (node.getCount() >= count_cutoff)
  {
    int this_count = 0;
    for (MegaTree::ChildIterator it(tree, node); !it.finished(); it.next())
    {
      numChildren(tree, it.getChildNode(), count_cutoff, num_children, count);
      this_count++;
    }
    count++;
    //printf("Call numchildren on node %s\n", node.toString().c_str());
    //printf("This count %d\n", this_count);
    num_children += this_count;
  }
}

static int process_queue_size = 0;  // TODO: For debugging, remove eventually
void getAllPointsRecursiveAsync(MegaTree &tree, NodeHandle& node, double resolution,
                                std::vector<double> &results, std::vector<double> &colors, megatree::List<NodeHandle*>& nodes_to_load)
{
  //printf("Get all points recursive for node %s with frame offset %f %f %f\n",
  //       node.getId().toString().c_str(), frame_offset[0], frame_offset[1], frame_offset[2]);

  assert(!node.isEmpty());

  // we hit a leaf
  if (node.isLeaf() || node.getNodeGeometry().getSize() <= resolution) 
  {
    double point[3];
    node.getPoint(point);
    results.push_back(point[0]);
    results.push_back(point[1]);
    results.push_back(point[2]);

    double color[3];
    node.getColor(color);
    colors.push_back(color[0]);
    colors.push_back(color[1]);
    colors.push_back(color[2]);
  }

  // need to descend further
  else 
  {
    for(int i = 0; i < 8; ++i)
    {
      if(node.hasChild(i))
      {
        megatree::NodeHandle* child = new megatree::NodeHandle;
        tree.getChildNode(node, i, *child);

        //if the child isn't valid, we'll push it onto our nodes to load list
        if(!child->isValid())
        {
          nodes_to_load.push_back(child);
          process_queue_size++;
        }
        else
        {
          getAllPointsRecursiveAsync(tree, *child, resolution, results, colors, nodes_to_load);
          tree.releaseNode(*child);
          delete child;
        }
      }
    }
  }
}

void queryRangeRecursiveAsync(MegaTree &tree, NodeHandle& node, 
                              const double* range_mid, const double* range_size, double resolution,
                              std::vector<double> &results, std::vector<double> &colors, megatree::List<NodeHandle*>& nodes_to_load)
{
  /*
     printf("Query recursive for node %s with %f points frame offset %f %f %f, range mid %f %f %f and size %f %f %f\n",
     node.getId().toString().c_str(), node.getCount(), frame_offset[0], frame_offset[1], frame_offset[2],
     range_mid[0], range_mid[1], range_mid[2], range_size[0], range_size[1], range_size[2]); 
     */

  // If the query range is outside of the node.
  if (nodeOutsideRange(node.getNodeGeometry(), range_mid, range_size))
  {
    // Completely outside.  Stop
  }
  else if (nodeInsideRange(node.getNodeGeometry(), range_mid, range_size)) 
  {
    // Completely inside
    getAllPointsRecursiveAsync(tree, node, resolution, results, colors, nodes_to_load);
  }

  // Partially intersecting
  else 
  { 
    // node is leaf
    if (node.isLeaf() || node.getNodeGeometry().getSize() <= resolution)
    {  
      queryRangeIntersecting(tree, node, range_mid, range_size, results, colors);
    }
    // node is not a leaf
    else   
    {
      // recurse to child if we can
      for(int i = 0; i < 8; ++i)
      {
        if(node.hasChild(i))
        {
          megatree::NodeHandle* child = new megatree::NodeHandle;
          tree.getChildNode(node, i, *child);

          //if the child isn't valid, we'll push it onto our nodes to load list
          if(!child->isValid())
          {
            nodes_to_load.push_back(child);
            process_queue_size++;
          }
          else
          {
            queryRangeRecursiveAsync(tree, *child, range_mid, range_size, resolution, results, colors, nodes_to_load);
            tree.releaseNode(*child);
            delete child;
          }
        }
      }
    }
  }
}

void rangeQueryLoop(MegaTree& tree, std::vector<double> lo, std::vector<double> hi, double resolution, std::vector<double>& results, std::vector<double>& colors)
{
  megatree::List<NodeHandle*> process_queue;

  megatree::NodeHandle* root = new megatree::NodeHandle;
  tree.getRoot(*root);
  assert(root->isValid());
  process_queue.push_back(root);
  process_queue_size = 1;

  double range_mid[3];
  range_mid[0] = (hi[0] + lo[0])/2;
  range_mid[1] = (hi[1] + lo[1])/2;
  range_mid[2] = (hi[2] + lo[2])/2;
  double range_size[3];
  range_size[0] = hi[0] - lo[0];
  range_size[1] = hi[1] - lo[1];
  range_size[2] = hi[2] - lo[2];

  while(!process_queue.empty())
  {
    megatree::NodeHandle* node_to_process = process_queue.front();

    //if the node isn't valid yet, we'll just move it to the back of the queue.
    if(!node_to_process->isValid())
    {
      process_queue.moveToBack(process_queue.getFrontPointer());
      node_to_process = NULL;
      //printf("%d\n", process_queue_size);
    }
    else
    {
      megatree::List<NodeHandle*> nodes_to_load;
      queryRangeRecursiveAsync(tree, *node_to_process, range_mid, range_size, resolution, results, colors, nodes_to_load);
      process_queue.pop_front();
      process_queue_size--;
      tree.releaseNode(*node_to_process);
      delete node_to_process;
      process_queue.spliceToBack(nodes_to_load);
      //printf("#%d\n", process_queue_size);
    }
  }
}


}
