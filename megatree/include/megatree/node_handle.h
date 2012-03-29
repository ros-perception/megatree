#ifndef MEGATREE_NODE_HANDLE_H
#define MEGATREE_NODE_HANDLE_H

#include <megatree/node.h>
#include <megatree/node_file.h>


namespace megatree
{


class NodeHandle
{
public:
  NodeHandle()
    : node(NULL), node_file(NULL), modified(false), new_family(false)
  {}

  ~NodeHandle()
  {
    if (node)
      fprintf(stderr, "NodeHandle destructed, even though it still has a node!\n");
  }

  void initialize(Node* node_p, const IdType& id_p, NodeFile* node_file_p, const NodeGeometry& node_geom_p)
  {
    new_family = false;
    modified = false;
    node = node_p;
    id = id_p;
    node_file = node_file_p;
    node_geom = node_geom_p;
  }
  

  bool isValid() const
  {
    // @TODO: is this checking too much??
    return (node_file != NULL && node_file->getNodeState() == LOADED && id.isValid());
  }

  bool hasNode() const
  {
    return node_file != NULL && node != NULL;
  }

  void invalidate()
  {
    node = NULL;
    node_file = NULL;
  }

  const IdType& getId() const
  {
    return id;
  }

  std::string toString() const;

  bool isModified() const
  {
    return modified;
  }

  bool isEmpty() const
  {
    return node->isEmpty();
  }


  Count getCount() const
  {
    return node->getCount();
  }

  bool isLeaf() const
  {
    return node->isLeaf();
  }

  bool hasChild(uint8_t i) const
  {
    return node->hasChild(i);
  }

  void setChild(uint8_t child) 
  {
    modified = true;

    node->setChild(child);
  }

  uint8_t getChildForNodePoint()
  {
    return node->getChildForNodePoint();
  }


  double* getPoint(double pnt[3]) const
  {
    return node->getPoint(node_geom, pnt);
  }


  double* getColor(double col[3]) const
  {
    return node->getColor(col);
  }

  float* getColor(float col[3]) const
  {
    return node->getColor(col);
  }

  void addPoint(const double* pt, const double* col)
  {
    modified = true;
    node->addPoint(node_geom, pt, col);
  }


  void setPoint(const double* pt, const double* col, Count cnt=1)
  {
    modified = true;
    node->setPoint(node_geom, pt, col, cnt);
  }


  bool operator==(const NodeHandle& nh) const
  {
    return *node == *(nh.node);
  }

  Node* getNode()
  {
    return node;
  }

  NodeFile* getNodeFile() const
  {
    return node_file;
  }

  const NodeGeometry& getNodeGeometry() const
  {
    return node_geom;
  }

  void copyFromChildNodes(NodeHandle children[8])
  {
    Node* node_children[8];
    for (unsigned i=0; i<8; i++)
      if (children[i].isValid())
        node_children[i] = children[i].getNode();
      else
        node_children[i] = NULL;
            

    node->copyFromChildNodes(node_children);
  }

  bool isNewFamily()
  {
    return new_family;
  }

  void waitUntilLoaded()
  {
    assert(node_file);
    node_file->waitUntilLoaded();
  }

  
private:
  Node* node;
  NodeGeometry node_geom;
  IdType id;
  NodeFile* node_file;
  bool modified, new_family;

};

}

#endif
