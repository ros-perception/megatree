#ifndef MEGATREE_H
#define MEGATREE_H

#include <cstdio>
#include <assert.h>
#include <vector>
#include <list>
#include <map>

#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "megatree/allocator.h"
#include "megatree/node_handle.h"
#include "megatree/node_file.h"
#include "megatree/list.h"
#include "megatree/function_caller.h"
#include <megatree/storage.h>
#include <fstream>
#include <tr1/unordered_set>
#include <megatree/blocking_queue.h>
#include <megatree/std_singleton_allocator.h>

namespace megatree
{
  const unsigned version = 11;
  const unsigned int CACHE_SIZE = 1000000;
  const float MIN_CELL_SIZE = 0.001; // 1 mm default accuray

  // Tree
  class MegaTree
  {
  public:
    // Loads the tree from disk, grabbing parameters from the metadata
    MegaTree(boost::shared_ptr<Storage> storage, unsigned cache_size, bool read_only);

    MegaTree(boost::shared_ptr<Storage> storage, const std::vector<double>& cell_center, const double& cell_size,
	     unsigned subtree_width, unsigned subfolder_depth,
	     unsigned cache_size=CACHE_SIZE, double _min_cell_size=MIN_CELL_SIZE);

    ~MegaTree();

    bool operator==(MegaTree& tree);

    // Use releaseNode() to deallocate nodes.
    void releaseNode(NodeHandle& node);

    // create nodes always blocking
    void createChildNode(NodeHandle& parent_node, uint8_t child, NodeHandle& child_node);
    NodeHandle* createChildNode(NodeHandle& parent_node, uint8_t child);

    // Methods for getting nodes.  The caller must release the returned node.
    void getRoot(NodeHandle &root_node);
    NodeHandle* getRoot();
    void getChildNode(const NodeHandle& parent_node, uint8_t child, NodeHandle &child_node);
    NodeHandle* getChildNode(const NodeHandle& parent_node, uint8_t child);

    // cache functions
    void flushCache();

    // Slow.  For careful debugging only.  Prints out which files still have nodes in use.
    void dumpNodesInUse();

    // Reset read/write/... counts that are used for debugging
    void resetCount()
    {
      count_hit = count_miss = count_file_write = count_nodes_read = 0;
    }

    // Get the total number of points in this tree
    unsigned long getNumPoints()
    {
      NodeHandle root;
      getRoot(root);
      unsigned long c = root.getCount();
      releaseNode(root);
      return c;
    }
  
    // Create a string with some statistics about this tree (read/write/cache_miss/...)
    std::string toString()
      {
	std::stringstream s;
	s << "Num nodes " << getNumPoints() << ", hit count " << count_hit << ", miss count " << count_miss  
	  << ", write count " << count_file_write << ", total nodes read " << count_nodes_read 
	  << ", nodes being written " << current_write_size
          << ", nodes per file " << (int)((float)current_cache_size / (float)file_cache.size())
	  << ", total cache usage " << (int)((float)current_cache_size / (float)max_cache_size * 100.0) << "%"
          << ", open files " << file_cache.size();
	return s.str();
      }

    // Get the geometry object of the root node
    const NodeGeometry& getRootGeometry() const
    {
      return root_geometry;
    }

    // Get the size of the smallest cell this tree supports
    double getMinCellSize() const { return min_cell_size; }


    // This function is pretty gross.  There should be a safer way to do this.
    void recenter(double x, double y, double z)
    {
      assert(getNumPoints() == 0);
      double new_center[] = {x, y, z};
      NodeGeometry new_root_geometry(new_center, root_geometry.getSize());
      root_geometry = new_root_geometry;
      writeMetaData();
    }

    // Only public for regression tests
    // Returns the ID of the file containing the given node.
    IdType getFileId(const IdType& node_id);
    ShortId getShortId(const IdType& node_id);


  private:
    unsigned cacheSize() const { return current_cache_size; }

    // callback after getAsync on storage finishes
    void readNodeFileCb(NodeFile* node_file, const ByteVec& buffer);

    // callback after putAsync on storage finishes when evicting node files
    void evictNodeFileCb(CacheIterator<IdType, NodeFile> it);

    // callback after putAsync on storage finishes when flushing node files to disk
    void flushNodeFileCb(CacheIterator<IdType, NodeFile> it, boost::mutex& mutex, boost::condition& condition, unsigned& remaining);

    void createRoot(NodeHandle &root);

    void initTree(boost::shared_ptr<Storage> storage, const std::vector<double>& _cell_center, const double& _cell_size,
		  unsigned _subtree_width, unsigned _subfolder_depth,
		  unsigned _cache_size, double _min_cell_size);


    // NodeFile methods
    // Returns the node file with id "file_id".  Should call releaseNodeFile on the returned pointer.
    NodeFile* createNodeFile(const IdType& file_id);
    NodeFile* getNodeFile(const IdType& file_id);
    void releaseNodeFile(NodeFile*& node_file);

    void cacheMaintenance();
    void writeMetaData();

    bool checkEqualRecursive(MegaTree& tree1, MegaTree& tree2, NodeHandle& node1, NodeHandle& node2);


    // storage backend
    boost::shared_ptr<Storage> storage;
    
    // cache properties
    boost::mutex file_cache_mutex;
    Cache<IdType, NodeFile> file_cache;  // front is most recenty used
    unsigned current_cache_size;  // Number of nodes currently in the cache.
    unsigned current_write_size;  // Number of nodes currently being written.

    // tree properties
    double min_cell_size;  // Minimum edge length of a cell in this tree
    NodeGeometry root_geometry;
    unsigned max_cache_size, subtree_width, subfolder_depth;
    boost::shared_ptr<Allocator<Node> > node_allocator;
    StdSingletonAllocatorInstance<std::_Rb_tree_node<std::pair<const ShortId, Node*> > >* singleton_allocator;
    // Counters that track tree statistics.
    unsigned count_hit, count_miss, count_file_write, count_nodes_read;

    bool read_only;


  public:
    class ChildIterator
    {
    public:
      ChildIterator(MegaTree& _tree, NodeHandle& parent, NodeFile* _children_file = NULL)
        : child_counter(-1), tree(_tree), children_file(_children_file)
      {
        if (!_children_file)
        {
          // get file for all children
          IdType child_id = parent.getId().getChild(0);
          IdType children_file_id = tree.getFileId(child_id);
          children_file = tree.getNodeFile(children_file_id);  
          children_file->waitUntilLoaded();
        }
      
        // get all children
        for (unsigned int i=0; i<8; i++)
        {
          if (parent.hasChild(i))
          {
            IdType child_id = parent.getId().getChild(i);
            NodeGeometry child_geometry = parent.getNodeGeometry().getChild(i);
            Node* child_node = children_file->readNode(tree.getShortId(child_id));
            children[i].initialize(child_node, child_id, children_file, child_geometry);
          }
        }
        if (!_children_file)
          children_file->removeUser();
      
        // get first child
        next();
      }
      
      
      ~ChildIterator()
      {
        for (unsigned int i=0; i<8; i++)
        {
          if (children[i].isValid())
            tree.releaseNode(children[i]);
        }
      }

      void next()
      {
        child_counter++;
        while (child_counter < 8 && !children[child_counter].isValid())
          child_counter++;
      }
      
      
      bool finished()
      {
        return (child_counter > 7);
      }
      
      uint8_t getChildNr()
      {
        return child_counter;
      }

      NodeHandle& getChildNode()
      {
        return children[child_counter];
      }

      NodeHandle* getAllChildren()
      {
        return  children;
      }

    private:
      NodeHandle children[8];
      int child_counter;
      MegaTree& tree;
      NodeFile* children_file;
    };

  };



}


#endif
