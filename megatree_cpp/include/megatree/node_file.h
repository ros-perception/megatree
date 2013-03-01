#ifndef MEGATREE_NODE_FILE_H_
#define MEGATREE_NODE_FILE_H_

#include <megatree/std_singleton_allocator.h>
#include <megatree/allocator.h>
#include <megatree/node.h>
#include <megatree/cache.h>
#include <megatree/storage.h>  // for ByteVec typedef
#include <megatree/tree_common.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/filesystem.hpp>
#include <pthread.h>


namespace megatree
{

class SpinLock
{
  public:
    SpinLock()
    {
      int ret = pthread_spin_init(&spinlock, 0);
      assert(ret == 0);
    }

    void lock()
    {
      int ret = pthread_spin_lock(&spinlock);
      assert(ret == 0);
    }

    void unlock()
    {
      int ret = pthread_spin_unlock(&spinlock);
      assert(ret == 0);
    }

    ~SpinLock()
    {
      pthread_spin_destroy(&spinlock);
    }

  class ScopedLock
  {
    public:
      ScopedLock(SpinLock& _spinlock): spinlock(_spinlock)
      {
        spinlock.lock();
      }

      ~ScopedLock()
      {
        spinlock.unlock();
      }

    private:
      SpinLock& spinlock;

  };

  private:
    pthread_spinlock_t spinlock;
};


class Cond
{
public:
  Cond(): cond(false)
  {
  }

  void wait(SpinLock& spinlock)
  {
    cond = false;
    spinlock.unlock();
    //TODO: There has just got to be a better way than having to write a custom
    //while loop here. The pthread_cond stuff won't take a pthread_spinlock as
    //input though and I cannot for the life of me figure out how to get the
    //mutex that, I think, is contained within the pthread_spinlock... ugh
    //swap this out for something better... please
    while(!cond)
      usleep(10);
    spinlock.lock();
  }

  void notify_all()
  {
    cond = true;
  }

private:
  bool cond;
};



enum NodeState
{
  INVALID,
  LOADING,
  LOADED,
  EVICTING
};



class NodeFile
{
  typedef Allocator<Node> NodeAllocator;
  typedef Allocator<std::pair<ShortId, Node*> > PairAllocator;

public:
  NodeFile(const boost::filesystem::path& _path,
           boost::shared_ptr<NodeAllocator> _node_allocator = boost::shared_ptr<NodeAllocator>(),
           boost::shared_ptr<PairAllocator> _pair_allocator = boost::shared_ptr<PairAllocator>())
  : node_state(LOADING), path(_path),
    child_files(0),
    node_allocator(_node_allocator), pair_allocator(_pair_allocator),
    use_count(0) {}

  ~NodeFile();

  // empty deserialize
  void deserialize();

  // deserializes a ByteVec into the node file
  void deserialize(const ByteVec& buffer);

  // serialized the node file into a ByteVec
  void serialize(ByteVec& buffer);

  // Serializes the node file into a ByteVec using the very tiny
  // "bytesize" format.  3 bytes for position, 3 for color.
  void serializeBytesize(ByteVec& buffer);

  // Reads an existing node from the cache.  Must return this node with releaseNode()
  Node* readNode(const ShortId& short_id);

  // Creates a new node in this file.  Must return this node with releaseNode()
  Node* createNode(const ShortId& short_id);

  void initializeFromChildren(const boost::filesystem::path &_path,
      std::vector<boost::shared_ptr<NodeFile> >& children);
  void initializeRootNodeFile(const boost::filesystem::path &_path, NodeFile& child);

  // Returns a node.
  void releaseNode(Node* node, const ShortId& short_id, bool modified);

  void setWritten()
  {
    is_modified = false;
  }

  // get the number of nodes that are currently in use.  Protected by mutex.
  unsigned users() const
  {
    return use_count;
  }

  bool hasChildFile(uint8_t child) const
  {
    return (child_files >> child) & 1;
  }

  void setChildFile(uint8_t child)
  {
    is_modified = true;
    child_files |= (1 << child);
  }

  void clearChildFile(uint8_t child)
  {
    is_modified = true;
    child_files &= ~(1 << child);
  }

  // Protected by mutex.
  void addUser()
  {
    use_count++;
  }

  // Protected by mutex.
  void removeUser()
  {
    assert(use_count > 0);
    use_count--;
  }

  boost::filesystem::path getPath() const { return path; }

  unsigned cacheSize() const
  {
    return node_cache.size();
  }

  bool isModified()
  {
    return is_modified;
  }

  NodeState getNodeState()
  {
    SpinLock::ScopedLock lock(node_state_mutex);
    return node_state;
  }

  void setNodeState(NodeState state);

  void waitUntilLoaded();

  boost::mutex mutex;

private:

  SpinLock node_state_mutex;
  //  boost::condition node_state_condition;
  Cond node_state_condition;
  NodeState node_state;

  boost::filesystem::path path;

  // Bitstring, indicating which child files exist.
  uint8_t child_files;

  // Lookup table, keyed on the node's short_id in the file.
  //
  // The node cache used to be an std::tr1::unordered_map, but was
  // switched to a map to get more deterministic memory usage.  The
  // unordered_map was eating memory by allocating buckets and
  // hashtable nodes (generally when createNode was called) and caused
  // the overall memory usage of import_las to grow over time, until
  // it used up all the machine's memory.  After changing to std::map,
  // the memory usage now remains flat after the initial cache fill.
  //
  // The memory usage of std::map is much better, and the speed did
  // not measurably change.
  typedef std::map<ShortId, Node*> NodeCache;
  //std::map<ShortId, Node*, std::less<ShortId>, StdSingletonAllocator <std::pair<const ShortId, Node*> > > node_cache;
  std::map<ShortId, Node*, std::less<ShortId> > node_cache;

  static void serializeNode(const Node* node, const ShortId& short_id, ByteVec& buffer, unsigned& offset);
  static void deserializeNode(Node* node, ShortId& short_id, const ByteVec& buffer, unsigned& offse);

  boost::shared_ptr<NodeAllocator> node_allocator;
  boost::shared_ptr<PairAllocator> pair_allocator;

  size_t use_count;
  bool is_modified;
};

}

#endif
;
