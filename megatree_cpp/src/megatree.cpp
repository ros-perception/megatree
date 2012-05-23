#include <megatree/megatree.h>
#include <megatree/tree_functions.h>
#include <megatree/metadata.h>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


namespace megatree
{

// Loads the tree from disk, grabbing parameters from the metadata
MegaTree::MegaTree(boost::shared_ptr<Storage> _storage, unsigned cache_size, bool _read_only)
  : storage(_storage), read_only(_read_only)
{
  printf("Reading existing tree\n");

  // get tree metadata
  ByteVec data;
  storage->get("metadata.ini", data);
  MetaData metadata;
  metadata.deserialize(data);

  // Checks that the on-disk version matches the code version.
  if (metadata.version != version)
  {
    fprintf(stderr, "You are trying to read a tree with version %d from disk, but your code was compiled for version %d\n",
            metadata.version, version);
    abort();
  }

  // get params
  min_cell_size = metadata.min_cell_size;
  subtree_width = metadata.subtree_width;
  subfolder_depth = metadata.subfolder_depth;

  // Initializes the tree
  initTree(storage, metadata.root_center, metadata.root_size, subtree_width, subfolder_depth, cache_size, min_cell_size);
}



MegaTree::MegaTree(boost::shared_ptr<Storage> _storage, const std::vector<double>& cell_center, const double& cell_size,
                   unsigned subtree_width, unsigned subfolder_depth,
                   unsigned cache_size, double min_cell_size)
  : storage(_storage), read_only(false)
{
  initTree(storage, cell_center, cell_size, subtree_width, subfolder_depth, cache_size, min_cell_size);

  // Creates the root node for this new tree.
  NodeHandle root;
  createRoot(root);
  releaseNode(root);

  // writes the metadata of this new tree
  writeMetaData();

}

MegaTree::~MegaTree()
{
  flushCache();

  while (true)
  {
    {
      // delete node file pointers
      boost::mutex::scoped_lock lock(file_cache_mutex);
      if (file_cache.size() == 0)
        break;

      CacheIterator<IdType, NodeFile> it = file_cache.iterate();
      while (!it.finished())
      {
        NodeFile* to_delete(NULL);
        {
          // lock node file
          boost::mutex::scoped_try_lock file_lock(it.get()->mutex);

          // delete node file and remove it form cache
          if (file_lock && it.get()->getNodeState() == LOADED)
          {
            CacheIterator<IdType, NodeFile> it_to_delete = it;
            it.next();
            to_delete = it_to_delete.get();
            file_cache.erase(it_to_delete);
            current_cache_size -= to_delete->cacheSize();
          }
          else
            it.next();
        }

        if (to_delete)
          delete to_delete;
      
      }
    }
    // sleep
    usleep(100000);
  }

  if (singleton_allocator) {
    singleton_allocator->destruct();
    delete singleton_allocator;
  }
}



void MegaTree::initTree(boost::shared_ptr<Storage> _storage, const std::vector<double>& _cell_center, const double& _cell_size,
                        unsigned _subtree_width, unsigned _subfolder_depth,
                        unsigned _cache_size, double _min_cell_size)
{
  storage = _storage;
  subtree_width = _subtree_width;
  subfolder_depth = _subfolder_depth;
  max_cache_size = _cache_size;
  current_cache_size = 0;
  current_write_size = 0;

  // reset counters
  resetCount();

  // Pre-allocates memory for the nodes.
  node_allocator.reset(new Allocator<Node>(_cache_size + _cache_size / 2));

  // TODO: the singleton allocator is disabled for now.  The mapreduce
  // programs couldn't run with it enabled, and tcmalloc does a decent
  // job of allocating for stl containers.
  //
  // Make sure to construct singleton allocator with the correct size before any of the node files start using it
  singleton_allocator = NULL; //StdSingletonAllocatorInstance<std::_Rb_tree_node<std::pair<const ShortId, Node*> > >::getInstance(_cache_size + _cache_size / 5);

  // compute tree min and max corners
  assert(_cell_center.size() == 3);
  root_geometry = NodeGeometry(_cell_center, _cell_size);
  min_cell_size = _min_cell_size;

  printf("Created tree with min cell size: %.4f, root (%lf, %lf, %lf)--(%lf, %lf, %lf), subtree width: %d, subfolder depth: %d\n",
         min_cell_size,
         root_geometry.getLo(0), root_geometry.getLo(1), root_geometry.getLo(2),
         root_geometry.getHi(0), root_geometry.getHi(1), root_geometry.getHi(2),
         subtree_width, subfolder_depth);
}
  



void MegaTree::writeMetaData()
{
  printf("Writing metadata of a new MegaTree\n");

  if (read_only)
  {
    fprintf(stderr, "You are trying to write metadata of a read-only tree\n");
    abort();
  }

  std::vector<double> root_center(3);
  root_center[0] = (root_geometry.getHi(0) + root_geometry.getLo(0)) / 2.0;
  root_center[1] = (root_geometry.getHi(1) + root_geometry.getLo(1)) / 2.0;
  root_center[2] = (root_geometry.getHi(2) + root_geometry.getLo(2)) / 2.0;
  MetaData metadata(version, subtree_width, subfolder_depth,
                    min_cell_size, root_geometry.getSize(), root_center);

  ByteVec data;
  metadata.serialize(data);
  storage->put("metadata.ini", data);
}




bool MegaTree::checkEqualRecursive(MegaTree& tree1, MegaTree& tree2, NodeHandle& node1, NodeHandle& node2)
{
  //if these nodes are not equal, just return false
  printf("%s  ----   %s\n", node1.toString().c_str(), node2.toString().c_str());
  if(!(node1 == node2))
  {
    return false;
  }

  //if the nodes are leaves, and we know they're already equal, we can just
  //return true
  if(node1.isLeaf())
  {
    return true;
  }

  // Loops through the children and recurses
  for(uint8_t i = 0; i < 8; ++i)
  {
    if(node1.hasChild(i))
    {
      NodeHandle child1;
      tree1.getChildNode(node1, i, child1);
      NodeHandle child2;
      tree2.getChildNode(node2, i, child2);

      child1.waitUntilLoaded();
      child2.waitUntilLoaded();
      bool child_is_equal = checkEqualRecursive(tree1, tree2, child1, child2);
      tree1.releaseNode(child1);
      tree2.releaseNode(child2);
      if (!child_is_equal)
        return false;
    }
  }

  return true;
}



bool MegaTree::operator==(MegaTree& tree)
{
  NodeHandle my_root;
  getRoot(my_root);
  NodeHandle other_root;
  tree.getRoot(other_root);
  bool equal = checkEqualRecursive(*this, tree, my_root, other_root);
  releaseNode(my_root);
  tree.releaseNode(other_root);
  return equal;
}



// create a new node file
NodeFile* MegaTree::createNodeFile(const IdType& file_id)
{
  // get file path
  std::string relative_path, filename;
  file_id.toPath(subfolder_depth, relative_path, filename);
  boost::filesystem::path path = boost::filesystem::path(relative_path) / filename;

  // Create a new NodeFile and add it to the cache
  NodeFile* file = new NodeFile(path, node_allocator);
  file->addUser();  // make sure file cannot get deleted in cache maintenance

  // The file doesn't exist, so we create it from scratch.
  file->deserialize();

  {
    // TODO: Don't we need to check that this file hasn't been added in the meantime by another thread?
    boost::mutex::scoped_lock lock(file_cache_mutex);
    file_cache.push_front(file_id, file);
  }

  cacheMaintenance();

  return file;
}


// internal method to get node file based on file id
NodeFile* MegaTree::getNodeFile(const IdType& file_id)
{
  NodeFile* file(NULL);

  {
    //lock the file cache
    boost::mutex::scoped_lock lock(file_cache_mutex);

    // get the file from the file cache
    Cache<IdType, NodeFile>::iterator it = file_cache.find(file_id);
    if (!it.finished())
    {
      count_hit++;
      
      file = it.get();
      
      //lock the file
      boost::mutex::scoped_lock file_lock(file->mutex);
      
      assert(file->getNodeState() != INVALID);
      
      //we need to check if this file is being evicted, and set the state to loading
      if(file->getNodeState() == EVICTING)
        file->setNodeState(LOADING);
      
      file->addUser();  // make sure file cannot get deleted in cache maintenance
      
      // Now that the write pointer is definitely elsewhere, moves
      // this file to the front on the LRU cache.
      file_cache.move_to_front(it);
    }
  }    
   
 
  // The file wasn't found in the cache, so we load it from storage.
  if (!file)
  {
    // get file path
    std::string relative_path, filename;
    file_id.toPath(subfolder_depth, relative_path, filename);
    boost::filesystem::path path = boost::filesystem::path(relative_path) / filename;

    // create new nodefile
    file = new NodeFile(path, node_allocator);
    file->addUser();  // make sure file cannot get deleted in cache maintenance
    
    // Async request to read the nodefile
    storage->getAsync(path, boost::bind(&MegaTree::readNodeFileCb, this, file, _1));
    
    // add nodefile to cache
    {
      boost::mutex::scoped_lock lock(file_cache_mutex);
      file_cache.push_front(file_id, file);
    }
    count_miss++;
  }

  return file;
}


// Allow the node file to get deleted in cache maintenance
void MegaTree::releaseNodeFile(NodeFile*& node_file)
{
  boost::mutex::scoped_lock lock(node_file->mutex);
  node_file->removeUser();
}



NodeHandle* MegaTree::createChildNode(NodeHandle& parent_node, uint8_t child)
{
  NodeHandle* nh = new NodeHandle;
  createChildNode(parent_node, child, *nh);
  return nh;
}


// Create a new node that does not exist yet
void MegaTree::createChildNode(NodeHandle& parent_node, uint8_t child, NodeHandle& child_node_handle)
{
  // get child info
  assert(!parent_node.hasChild(child));

  IdType child_id = parent_node.getId().getChild(child);
  IdType child_file_id = getFileId(child_id);
  NodeGeometry child_geometry = parent_node.getNodeGeometry().getChild(child);

  // Checks if the child node file exists.  If it doesn't exist, then
  // we don't need to hit storage to load it.
  NodeFile* child_file = NULL;
  IdType parent_file_id = getFileId(parent_node.getId());
  NodeFile* parent_node_file = getNodeFile(parent_file_id);
  parent_node_file->waitUntilLoaded();
  assert(parent_node_file->getNodeState() == LOADED);

  // Checks if the child file exists, without going to disk
  uint8_t which_child_file = child_file_id.getChildNr();
  if (child_file_id.isRootFile() || parent_node_file->hasChildFile(which_child_file))
  {
    child_file = getNodeFile(child_file_id);  
    child_file->waitUntilLoaded();  // always blocking for creating nodes
  }
  else
  {
    // Fast creation for non-existant node files.
    child_file = createNodeFile(child_file_id);
    // tell parent file it has a new child file
    parent_node_file->setChildFile(which_child_file);
  }
  releaseNodeFile(parent_node_file);

  // tell parent node it has a new child node
  parent_node.setChild(child);

  // create new node and node handle
  Node* child_node = child_file->createNode(getShortId(child_id));
  child_node_handle.initialize(child_node, child_id, child_file, child_geometry);

  current_cache_size++;

  releaseNodeFile(child_file);  // we have a Node from this file, so unlock file
}

NodeHandle* MegaTree::getChildNode(const NodeHandle& parent_node, uint8_t child)
{
  NodeHandle* nh = new NodeHandle;
  getChildNode(parent_node, child, *nh);
  return nh;
}

// Get a child node form a parent node
void MegaTree::getChildNode(const NodeHandle& parent_node, uint8_t child, NodeHandle& child_node_handle)
{
  // get child info
  assert(parent_node.hasChild(child));
  IdType child_id = parent_node.getId().getChild(child);
  IdType child_file_id = getFileId(child_id);
  NodeGeometry child_geometry = parent_node.getNodeGeometry().getChild(child);

  // retrieve the child from the nodefile
  NodeFile* child_file = getNodeFile(child_file_id);  

  // unlock file after we got a node from the file
  Node* child_node = NULL;
  {
    boost::mutex::scoped_lock lock(child_file->mutex);
    child_node = child_file->readNode(getShortId(child_id));
  }
  child_node_handle.initialize(child_node, child_id, child_file, child_geometry);

  // DEBUGGING CODE
  if (!child_file_id.isRootFile())
  {
    const NodeFile* parent_file = parent_node.getNodeFile();
    unsigned which_child_file = child_file_id.getChildNr();
    if (!parent_file->hasChildFile(which_child_file))
    {
    fprintf(stderr, "Parent file of %s does not know is has child nr %d, child id is %s\n",
            child_file_id.toString().c_str(), which_child_file, child_id.toString().c_str());
    }
  }


  releaseNodeFile(child_file);
}



void MegaTree::releaseNode(NodeHandle& node_handle)
{
  if(!node_handle.hasNode())
  {
    fprintf(stderr, "Trying to release a node_handle that doesn't have a node\n");
  }
  else
  {

    boost::mutex::scoped_lock lock(node_handle.getNodeFile()->mutex);
    // release node inside nodehandle
    node_handle.getNodeFile()->releaseNode(node_handle.getNode(), getShortId(node_handle.getId()), node_handle.isModified());
  }

  // Invalidates the NodeHandle
  node_handle.invalidate();
}





void MegaTree::flushCache()
{
  boost::mutex::scoped_lock lock(file_cache_mutex);
  boost::condition condition;  
  boost::mutex mutex;

  // iterate over all files, and write data
  printf("Flushing %d files...\n", (int)file_cache.size());
  unsigned remaining = 0;
  for (CacheIterator<IdType, NodeFile> file_it = file_cache.iterate(); !file_it.finished(); file_it.next())
  {
    //The file needs to be locked when it is written
    boost::mutex::scoped_lock file_lock(file_it.get()->mutex);

    if (file_it.get()->getNodeState() != LOADING && file_it.get()->isModified())
    {
      if (read_only)
      {
        fprintf(stderr, "You are trying to write node files of a read-only tree\n");
        abort();
      }

      {
        boost::mutex::scoped_lock remaining_lock(mutex);
        remaining++;
      }

      // get data to write
      ByteVec byte_data;
      file_it.get()->serialize(byte_data);
      
      // start asynchronous writing
      storage->putAsync(file_it.get()->getPath(), byte_data, 
                        boost::bind(&MegaTree::flushNodeFileCb, this, file_it, boost::ref(mutex), boost::ref(condition), boost::ref(remaining)));
    }
  }

  // wait for async writing to finish
  boost::mutex::scoped_lock remaining_lock(mutex);
  if (remaining > 0)
    condition.wait(remaining_lock);

  printf("Finished flushing %d files\n", (int)file_cache.size());
}



//manage the cache
void MegaTree::cacheMaintenance()
{
  //A list of node files that we want to be written
  std::vector<CacheIterator<IdType, NodeFile> > write_list;

  {
    // Locks the file cache
    boost::mutex::scoped_lock lock(file_cache_mutex);

    // Starts evicting at the back of the LRU cache.
    Cache<IdType, NodeFile>::iterator it = file_cache.iterateBack();

    // How many nodes we need to evict
    int number_to_evict = current_cache_size - max_cache_size - current_write_size;
    int number_evicted = 0;

    //we want to remove nodes from the cache until we're under our overflow
    while(!it.finished() && number_to_evict > number_evicted)
    {
      NodeFile* delete_file(NULL);
      {
        boost::mutex::scoped_try_lock file_lock(it.get()->mutex);
        //if the file is unavailable for eviction, we'll just skip it
        if(!file_lock || it.get()->getNodeState() != LOADED || it.get()->users() > 0)
        {
          // skip file
          it.previous();
        }
        else if(!it.get()->isModified())
        {
          // immediately evict file
          delete_file = it.get();  // postpone delete
          current_cache_size -= it.get()->cacheSize();
          number_evicted += it.get()->cacheSize();
          Cache<IdType, NodeFile>::iterator evicted_it = it;
          it.previous();
          file_cache.erase(evicted_it);
        }
        else
        {
          if (read_only)
          {
            fprintf(stderr, "You are trying to write node files of a read-only tree\n");
            abort();
          }

          // start evicting this file
          it.get()->setNodeState(EVICTING);

          // get data to write
          ByteVec byte_data;
          it.get()->serialize(byte_data);

          // Start asynchronous writing
          number_evicted += it.get()->cacheSize();
          current_write_size += it.get()->cacheSize();
          storage->putAsync(it.get()->getPath(), byte_data, boost::bind(&MegaTree::evictNodeFileCb, this, it));
          it.previous();
        }
      }
      if (delete_file)
        delete delete_file;
    }

    if(number_to_evict >= number_evicted && it.finished())
    {
      fprintf(stderr, "We are out of space in our cache but cannot evict any more\n");
      abort();
    }
  }
}




// get the file_id and node short_id from the node_id
IdType MegaTree::getFileId(const IdType& node_id)
{
  return node_id.getParent(subtree_width);
}


ShortId MegaTree::getShortId(const IdType& node_id)
{
  unsigned shift = std::min(node_id.level(), subtree_width);
  return node_id.getBits(shift*3);
}


void MegaTree::createRoot(NodeHandle &root_node_handle)
{
  // get root node info
  IdType root_id = IdType(1L);
  IdType root_file_id = getFileId(root_id);

  // the nodefile will create an pointer for this child
  NodeFile* root_file = createNodeFile(root_file_id);
  Node* root_node = root_file->createNode(getShortId(root_id));
  assert(getShortId(root_id) == 1);

  root_node_handle.initialize(root_node, root_id, root_file, root_geometry);

  current_cache_size++;
  root_file->removeUser();
}

void MegaTree::getRoot(NodeHandle &root_node_handle)
{
  // get root node info
  IdType root_id = IdType(1L);
  IdType root_file_id = getFileId(root_id);

  // retrieve the root node from the nodefile
  NodeFile* root_file = getNodeFile(root_file_id);
  root_file->waitUntilLoaded();    // always blocking

  Node* root_node = NULL;
  {
    boost::mutex::scoped_lock lock(root_file->mutex);
    root_node = root_file->readNode(getShortId(root_id));
  }

  root_node_handle.initialize(root_node, root_id, root_file, root_geometry);
  root_file->removeUser();
}

NodeHandle* MegaTree::getRoot()
{
  NodeHandle* nh = new NodeHandle;
  getRoot(*nh);
  return nh;
}

void MegaTree::dumpNodesInUse()
{
  boost::mutex::scoped_lock lock(file_cache_mutex);

  printf("Nodes in use:\n");
  for (Cache<IdType, NodeFile>::ObjListIterator it(file_cache.list_.frontIterator()); !it.finished(); it.next())
  {
    if (it.get().object->users() > 0) {
      printf("    %3d %s\n", it.get().object->users(), it.get().id.toString().c_str());
    }
  }
}



void MegaTree::flushNodeFileCb(CacheIterator<IdType, NodeFile> it, boost::mutex& mutex, boost::condition& condition, unsigned& remaining)
{
  // Locks the node file
  boost::mutex::scoped_lock file_lock(it.get()->mutex);

  // File is written 
  it.get()->setWritten();
  count_file_write++;

  // Locks the remaining count
  boost::mutex::scoped_lock lock(mutex);
  remaining--;
  if (remaining == 0)
    condition.notify_one();
}



void MegaTree::evictNodeFileCb(CacheIterator<IdType, NodeFile> it)
{
  NodeFile* delete_file(NULL);
  {
    // Locks the node file
    boost::mutex::scoped_lock file_lock(it.get()->mutex);
    NodeState state = it.get()->getNodeState();

    // File is written 
    assert(it.get()->isModified());
    it.get()->setWritten();
    count_file_write++;


    // File is no longer in use.  Removes it from the cache.
    if (state == EVICTING)
    {
      it.get()->setNodeState(INVALID);
      delete_file = it.get();  // postpone delete

      // update file cache
      boost::mutex::scoped_lock lock(file_cache_mutex);
      current_cache_size -= it.get()->cacheSize();
      current_write_size -= it.get()->cacheSize();
      file_cache.erase(it);
    }

    // The file was requested again while we were writing it.  Returns the file to the loaded state.
    else
    {
      assert(state == LOADING);
      it.get()->setNodeState(LOADED);

      // update file cache
      boost::mutex::scoped_lock lock(file_cache_mutex);
      current_write_size -= it.get()->cacheSize();   
    }
  }

  if (delete_file)
    delete delete_file; // delete pointer outside of node file lock

}



void MegaTree::readNodeFileCb(NodeFile* node_file, const ByteVec& buffer)
{
  {
    boost::mutex::scoped_lock lock(node_file->mutex);
    node_file->deserialize(buffer);
    current_cache_size += node_file->cacheSize();
  }
  cacheMaintenance();
}



}
