#ifndef MEGATREE_NODE_H
#define MEGATREE_NODE_H

#include <megatree/long_id.h>
#include <megatree/tree_common.h>
#include <megatree/node_geometry.h>
#include <sstream>
#include <vector>


namespace megatree
{

class NodeFile;

  static const double BITS_D = 65536.0;
  static const double BITS_2_D = 32768.0;
  static const uint64_t BITS = 65536;
  static const uint64_t BITS_2 = 32768;


// Node
class Node
{
// nodefile gets access to private variables to easily construct a node
friend class NodeFile;
friend class NodeHandle;


public:
  Node()
    :count(0), children(0)
  {
    point[0] = point[1] = point[2] = 0;
    color[0] = color[1] = color[2] = 0;
  }


  bool isEmpty() const
  {
    return count == 0;
  }


  Count getCount() const
  {
    return count;
  }


  bool isLeaf() const
  {
    return children == 0;
  }


  bool hasChild(uint8_t i) const
  {
    assert(i < 8);
    return (children >> i) & 1;
  }

  void setChild(uint8_t i)
  {
    assert(i < 8);
    children |= (1 << i);
  }


  void copyFromChildNodes(Node* child_nodes[8]) 
  {
    children = 0;
    count = 0;
    uint64_t sum_point[3] = {0,0,0};
    uint64_t sum_color[3] = {0,0,0};

    for (unsigned int i = 0; i < 8; i++)
    {
      if (child_nodes[i])
      {
        children += (1 << i);
        //printf("using %s\n", child_nodes[i]->toString().c_str());

        const Count& child_cnt = child_nodes[i]->count;
        Point* child_pnt = child_nodes[i]->point;
        Color* child_col = child_nodes[i]->color;
        
        uint64_t child_offset[3];
        getChildBitOffset(i, child_offset);

        sum_point[0] += child_cnt * (child_pnt[0] + child_offset[0]);
        sum_point[1] += child_cnt * (child_pnt[1] + child_offset[1]);
        sum_point[2] += child_cnt * (child_pnt[2] + child_offset[2]);

        sum_color[0] += child_cnt * child_col[0];
        sum_color[1] += child_cnt * child_col[1];
        sum_color[2] += child_cnt * child_col[2];

        // TODO: sum will overflow at some point.
        count += child_cnt;
      }
    }

    //we need to shift back down because we go up a level higher than we store
    //things to compute the average, getChildBitOffset holds the shifts that
    //bump things up, perhaps that should change at some point
    point[0] = (sum_point[0] / count) >> 1;
    point[1] = (sum_point[1] / count) >> 1;
    point[2] = (sum_point[2] / count) >> 1;

    color[0] = sum_color[0] / count;
    color[1] = sum_color[1] / count;
    color[2] = sum_color[2] / count;

    //printf("result %s\n", toString().c_str());
  }


  uint8_t getChildForNodePoint()
  {
    return  (point[0] >= BITS_2) << X_BIT | 
            (point[1] >= BITS_2) << Y_BIT | 
            (point[2] >= BITS_2) << Z_BIT;
  }



  void copyToChildNode(uint8_t child, Node* child_node) const
  {
    if (child & (1 << X_BIT))
      assert(point[0] >= BITS_2);
    else
      assert(point[0] < BITS_2);
    if (child & (1 << Y_BIT))
      assert(point[1] >= BITS_2);
    else
      assert(point[1] < BITS_2);
    if (child & (1 << Z_BIT))
      assert(point[2] >= BITS_2);
    else
      assert(point[2] < BITS_2);

    //just drop the most significant bit to drop the point down from the parent
    //to child bucket
    child_node->point[0] = point[0] << 1;
    child_node->point[1] = point[1] << 1;
    child_node->point[2] = point[2] << 1;

    child_node->color[0] = color[0];
    child_node->color[1] = color[1];
    child_node->color[2] = color[2];
    child_node->count = count;

    /*
    printf("Copied node %d %d %d into child %d: %d %d %d\n",
           (unsigned)(point[0]), (unsigned)(point[1]), (unsigned)(point[2]),
           child, (unsigned)(child_node->point[0]), (unsigned)(child_node->point[1]), (unsigned)(child_node->point[2]));
    */
  }



  void getChildBitOffset(uint8_t child, double* child_position) const
  {
    child_position[0] = (child & (1 << X_BIT)) ? BITS_D : 0;
    child_position[1] = (child & (1 << Y_BIT)) ? BITS_D : 0;
    child_position[2] = (child & (1 << Z_BIT)) ? BITS_D : 0;
  }

  void getChildBitOffset(uint8_t child, uint64_t* child_position) const
  {
    child_position[0] = (child & (1 << X_BIT)) ? BITS : 0;
    child_position[1] = (child & (1 << Y_BIT)) ? BITS : 0;
    child_position[2] = (child & (1 << Z_BIT)) ? BITS : 0;
  }


  double* getPoint(const NodeGeometry& ng, double pnt[3]) const
  {
    fixed_to_float(ng, point, pnt);
    return pnt;
  }


  double* getColor(double col[3]) const
  {
    col[0] = (double)color[0];
    col[1] = (double)color[1];
    col[2] = (double)color[2];

    return col;
  }

  float* getColor(float col[3]) const
  {
    col[0] = (float)color[0];
    col[1] = (float)color[1];
    col[2] = (float)color[2];

    return col;
  }

  void addPoint(const NodeGeometry& ng, const double* pt_float, const double* col)
  {
    assert(ng.contains(pt_float));

    // Converts to internal fixed-precision representation.
    Point pt[3];
    float_to_fixed(ng, pt_float, pt);

    // Computes the moving average.  Note that the points are discretized, so this computation is somewhat incorrect.
    count++;
    double f = 1.0 / (double)count;
    point[0] = (1.0 - f) * (double)(point[0])  + f * (double)(pt[0]);
    point[1] = (1.0 - f) * (double)(point[1])  + f * (double)(pt[1]);
    point[2] = (1.0 - f) * (double)(point[2])  + f * (double)(pt[2]);
    color[0] = (1.0 - f) * (double)(color[0])  + f * col[0];
    color[1] = (1.0 - f) * (double)(color[1])  + f * col[1];
    color[2] = (1.0 - f) * (double)(color[2])  + f * col[2];

  }


  void setPoint(const NodeGeometry& ng, const double* pt, const double* col, Count cnt=1)
  {
    if (!ng.contains(pt))
    {
      fprintf(stderr, "Node::setPoint()  Point (%lf, %lf, %lf) is not inside the geometry "
              "(%lf, %lf, %lf)--(%lf, %lf, %lf)\n", pt[0], pt[1], pt[2],
              ng.getLo(0), ng.getLo(1), ng.getLo(2), ng.getHi(0), ng.getHi(1), ng.getHi(2));
      abort();
    }

    count = cnt;
    float_to_fixed(ng, pt, point);
    color[0] = col[0];
    color[1] = col[1];
    color[2] = col[2];
  }

  bool operator==(const Node& n) const
  {
    return point[0] == n.point[0] &&
           point[1] == n.point[1] &&
           point[2] == n.point[2] &&
           color[1] == n.color[0] &&
           color[1] == n.color[1] &&
           color[2] == n.color[2] &&
           count == n.count &&
           children == n.children;
  }
  
  void reset()
  {
    count = 0;
    children = 0;
  }

  Node& operator =(const Node& n)
  {
    count = n.count;
    point[0] = n.point[0];  point[1] = n.point[1];  point[2] = n.point[2];
    color[0] = n.color[0];  color[1] = n.color[1];  color[2] = n.color[2];
    children = n.children;
    
    return *this;
  }

private:
  Count count;
  Point point[3];
  Color color[3];
  
  // which children exist
  uint8_t children;


  // Conversions between external floating point representation and
  // internal fixed precision representation.
  void float_to_fixed(const NodeGeometry &ng, const double pt_float[3], Point pt_fixed[3]) const
  {
    // Scales to [0,1), then to [0,BITS_D), then truncates to the integral type.
    //                   |<--               [0, BITS_D)                                 -->|
    //                             |<--     [0, 1)                                      -->|
    //                             |<--     [0, hi-lo)    -->|
    pt_fixed[0] = (Point)(BITS_D * (pt_float[0] - ng.getLo(0)) / (ng.getHi(0) - ng.getLo(0)));
    pt_fixed[1] = (Point)(BITS_D * (pt_float[1] - ng.getLo(1)) / (ng.getHi(1) - ng.getLo(1)));
    pt_fixed[2] = (Point)(BITS_D * (pt_float[2] - ng.getLo(2)) / (ng.getHi(2) - ng.getLo(2)));
  }
  void fixed_to_float(const NodeGeometry &ng, const Point pt_fixed[3], double pt_float[3]) const
  {
    // Scales from [0, BITS_D) to [0, 1) to [lo, hi)
    //                           |<--   [0, 1)  (shifted by 0.5)  -->|
    //                          |<--    [0, hi-lo)                                              -->| 
    pt_float[0] = ng.getLo(0) + ((double(pt_fixed[0]) + 0.5) / BITS_D) * (ng.getHi(0) - ng.getLo(0));
    pt_float[1] = ng.getLo(1) + ((double(pt_fixed[1]) + 0.5) / BITS_D) * (ng.getHi(1) - ng.getLo(1));
    pt_float[2] = ng.getLo(2) + ((double(pt_fixed[2]) + 0.5) / BITS_D) * (ng.getHi(2) - ng.getLo(2));
  }
};


}
#endif
