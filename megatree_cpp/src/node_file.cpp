#include <megatree/node_file.h>
#include <math.h>
#include <algorithm>
#include <string.h>

namespace megatree
{


void NodeFile::deserialize()
{
  is_modified = true;

  // signal conditions that are waiting for initialization
  SpinLock::ScopedLock lock(node_state_mutex);
  node_state = LOADED;
  node_state_condition.notify_all();
}


void NodeFile::deserialize(const ByteVec &buffer)
{
  //use_count = 0; only set use_count to 0 in constructor
  is_modified = false;
  unsigned offset = 0;

  // Reads the byte indicating which child node files exist.
  memcpy(&child_files, (void*)&buffer[offset], 1);
  offset += 1;

  // Reads all the nodes
  while (offset < buffer.size())
  {
    ShortId short_id;
    Node* node = node_allocator ? node_allocator->allocate() : new Node;
    deserializeNode(node, short_id, buffer, offset);

    // add node to cache
    NodeCache::iterator it = node_cache.find(short_id);
    if (it == node_cache.end())
    {
      node_cache.insert(std::pair<ShortId, Node* >(short_id, node));
    }
    else
    {
      *(it->second) = *node;
      node_allocator ? node_allocator->deAllocate(node) : delete node;
    }
  }

  assert(buffer.size() == offset);

  // signal conditions that are waiting for initialization
  SpinLock::ScopedLock lock(node_state_mutex);
  node_state = LOADED;
  node_state_condition.notify_all();

  //printf("Deserialized buffer %s with num nodes %d\n", path.string().c_str(), (int)node_cache.size());
}


void NodeFile::serialize(ByteVec& buffer)
{
  buffer.resize(1 + node_cache.size() * NODE_SIZE);
  unsigned offset = 0;

  // Writes the byte indicating which child node files exist.
  memcpy(&buffer[offset], (char*)(&child_files), 1);
  offset += 1;

  // write entire cache into buffer
  for (NodeCache::const_iterator it=node_cache.begin(); it!=node_cache.end(); it++)
    serializeNode(it->second, it->first, buffer, offset);

  //printf("Serialized to buffer %s with num nodes %d\n", path.string().c_str(), (int)node_cache.size());
}

void NodeFile::serializeBytesize(ByteVec& buffer)
{
  const static size_t STRIDE = 3 + 3;
  buffer.resize(1 + node_cache.size() * STRIDE);
  buffer[0] = child_files;
  size_t offset = 1;

  for (NodeCache::const_iterator it = node_cache.begin(); it != node_cache.end(); ++it)
  {
    // Determines the node's position from the id
    //
    // ID:  0123456789abcdefgh  (0 is high bit)
    //  x:  0  3  6  9  c  f
    //  y:   1  4  7  a  d  g
    //  z:    2  5  8  b  e  h
    uint8_t x=0, y=0, z=0;
    for (int i = 7; i >=0; --i) {
      int which = (it->first >> (i*3)) & 07;
      x = (x << 1) | ((which & (1<<X_BIT)) ? 1 : 0);
      y = (y << 1) | ((which & (1<<Y_BIT)) ? 1 : 0);
      z = (z << 1) | ((which & (1<<Z_BIT)) ? 1 : 0);
    }

    // TODO: This assumes nodefiles with a width of 6, which is not true all the time.
    x = x << 2;
    y = y << 2;
    z = z << 2;

    buffer[offset + 0] = x;
    buffer[offset + 1] = y;
    buffer[offset + 2] = z;

    // Copies over the color
    buffer[offset + 3] = it->second->color[0];
    buffer[offset + 4] = it->second->color[1];
    buffer[offset + 5] = it->second->color[2];

    offset += STRIDE;
  }
}


// use condition variable to wait for initialization
void NodeFile::waitUntilLoaded()
{
  //TODO: Decide what to do with conditions here
  //boost::mutex dummy_mutex;
  //boost::mutex::scoped_lock dummy_lock(dummy_mutex);
  while (true)
  {
    {
      SpinLock::ScopedLock lock(node_state_mutex);
      if(node_state == LOADED)
        break;
      node_state_condition.wait(node_state_mutex);
    }
    //node_state_condition.wait(dummy_lock);
  }
}

void NodeFile::setNodeState(NodeState state)
{
  SpinLock::ScopedLock lock(node_state_mutex);
  node_state = state;
  node_state_condition.notify_all();
}

NodeFile::~NodeFile()
{
  // return nodes back to node_allocator
  if (node_allocator) {
    std::vector<Node*> allocated_nodes(node_cache.size());
    size_t i = 0;
    for (NodeCache::iterator it = node_cache.begin(); it != node_cache.end(); it++)
    {
      allocated_nodes[i++] = it->second;
    }
    assert(!allocated_nodes.empty());
    assert(allocated_nodes[0] != NULL);
    node_allocator->deallocateMany(allocated_nodes);
  }
  else {
    for (NodeCache::iterator it = node_cache.begin(); it != node_cache.end(); it++)
    {
      delete it->second;
    }
  }

  // checks
  if (is_modified)
    fprintf(stderr, "Destructing NodeFile %s that was not written to disk\n", path.string().c_str());

  if (users() != 0)
    fprintf(stderr, "NodeFile %s destructing while %d nodes are still in use!\n", path.string().c_str(), users());
}




// Reads an existing node from the cache.
// When reading from disk, we don't know the id of the node, so the id is passed on.
Node* NodeFile::readNode(const ShortId& short_id)
{
  // get the node from the cache
  NodeCache::iterator it = node_cache.find(short_id);
  if (it == node_cache.end())
  {
    SpinLock::ScopedLock lock(node_state_mutex);

    assert(node_state != EVICTING);

    // allocate a Node object
    if (node_state == LOADING)
    {
      Node* node = node_allocator ? node_allocator->allocate() : new Node;
      node->reset();
      node_cache.insert(std::pair<ShortId, Node* >(short_id, node));
      use_count++;

      return node;
    }

    // bad situation
    fprintf(stderr, "Could not find node with short_id %o in %s with %d nodes\n",
            short_id, path.string().c_str(), (int)node_cache.size());
    /*
    for (NodeCache::iterator jt = node_cache.begin(); jt != node_cache.end(); ++jt)
    {
      fprintf(stderr, "  Node  %8u: \n", jt->first);
    }
    */
    //abort();
    return NULL;
  }

  use_count++;

  return it->second;
}

void NodeFile::initializeFromChildren(const boost::filesystem::path &_path,
                                      std::vector<boost::shared_ptr<NodeFile> >& children)
{
  assert(children.size() == 8);
  node_cache.clear();
  child_files = 0;
  path = _path;

  // Here's how the ID's of the nodes change:
  //
  //          nodefile       short_id
  // child:   f171114232?    321567
  // parent:  f171114232    ?32156

  // Assigns child nodes to the parent node that they descend from.
  typedef std::map<ShortId, std::vector<Node*> > ParentGrouping;
  ParentGrouping parent_groupings;  // parent short id  ->  8 child nodes
  for (int i = 0; i < 8; ++i)
  {
    if (children[i])
    {
      child_files |= (1 << i);

      // Encodes the id element that came from the child nodefile's id and
      // should prefix the short id of the parent nodes.
      //
      // TODO: This assumes that the node width is 6, an unreasonable
      // assumption to make at this level.  This must be changed eventually.
      ShortId short_id_prefix = i << (5 * 3);

      for (NodeCache::iterator it = children[i]->node_cache.begin(); it != children[i]->node_cache.end(); ++it)
      {
        uint8_t which_child = it->first & 7;

        // Determines the parent node for this node
        ShortId parent_short_id = short_id_prefix + (it->first >> 3);

        // Puts the child into its parent's group.
        std::vector<Node*> &children = parent_groupings[parent_short_id];
        if (children.empty())
          children.resize(8, NULL);
        children[which_child] = it->second;
      }
    }
  }

  // Condenses the groups of child nodes into a parent node.
  for (ParentGrouping::iterator it = parent_groupings.begin(); it != parent_groupings.end(); ++it)
  {
    Node *parent_node = new Node;
    parent_node->copyFromChildNodes(&it->second[0]);
    node_cache.insert(std::make_pair(it->first, parent_node));
  }
}

// Special case: Initializes NodeFile f from f1
void NodeFile::initializeRootNodeFile(const boost::filesystem::path &_path, NodeFile& child)
{
  path = _path;  // A formality.  If pretty much has to be "f"

  node_cache.clear();
  path = _path;
  child_files = 0x02;

  NodeCache last_level = child.node_cache;
  bool condensing_f1 = true;

  while (true)
  {
    printf("Looping\n");
    //   1. Groups the nodes in the last level by parent.

    typedef std::map<ShortId, std::vector<Node*> > ParentGrouping;
    ParentGrouping parent_groupings;  // parent short id  ->  8 child nodes

    // The nodes from f1 need a "1" prefixed to their short_ids.
    //
    // TODO: This assumes that the node width is 6, an unreasonable
    // assumption to make at this level.  This must be changed eventually.
    ShortId short_id_prefix = 0;
    if (condensing_f1)
      short_id_prefix = 1 << (5 * 3);

    for (NodeCache::iterator it = last_level.begin(); it != last_level.end(); ++it)
    {
      uint8_t which_child = it->first & 7;

      // Determines the parent node for this node
      ShortId parent_short_id = short_id_prefix + (it->first >> 3);

      // Puts the child into its parent's group.
      std::vector<Node*> &children = parent_groupings[parent_short_id];
      if (children.empty())
        children.resize(8, NULL);
      children[which_child] = it->second;

      printf("child %o is %u of %o\n", it->first, which_child, parent_short_id);
    }

    //     2. Condenses the groups of child nodes into parent nodes.

    last_level.clear();
    for (ParentGrouping::iterator it = parent_groupings.begin(); it != parent_groupings.end(); ++it)
    {
      Node *parent_node = new Node;
      parent_node->copyFromChildNodes(&it->second[0]);
      last_level.insert(std::make_pair(it->first, parent_node));
    }

    //     3. Inserts the parent nodes into nodefile f

    node_cache.insert(last_level.begin(), last_level.end());

    condensing_f1 = false;

    // Checks if we've reached the root.
    if (last_level.size() == 1 && last_level.begin()->first == 1)
      break;
  }
}


// Create a new node.
Node* NodeFile::createNode(const ShortId& short_id)
{
  //printf("Create node with short_id %d in %s/%s \n", short_id, folder.string().c_str(), filename.c_str());
  //assert(node_cache.find(short_id) == node_cache.end());

  // Creates a new node object and adds it to the cache
  Node* node = node_allocator ? node_allocator->allocate() : new Node;
  node->reset();
  node_cache.insert(std::pair<ShortId, Node* >(short_id, node));

  use_count++;
  is_modified = true;
  return node;
}


void NodeFile::releaseNode(Node* node, const ShortId& short_id, bool modified)
{
  assert(use_count > 0);  // Sanity check that we're not releasing more nodes than were created.
  //assert(node_cache.find(short_id) != node_cache.end());

  is_modified = is_modified || modified;
  use_count--;
}



// write node to buffer
void NodeFile::serializeNode(const Node* node, const ShortId& short_id, ByteVec& buffer, unsigned& offset)
{
  memcpy(&buffer[offset], (char *)node->point, POINT_SIZE);
  offset += POINT_SIZE;
  memcpy(&buffer[offset], (char*)node->color, COLOR_SIZE);
  offset += COLOR_SIZE;
  memcpy(&buffer[offset], (char *)&node->count, COUNT_SIZE);
  offset += COUNT_SIZE;
  memcpy(&buffer[offset], (char *)&node->children, CHILDREN_SIZE);
  offset += CHILDREN_SIZE;
  memcpy(&buffer[offset], (char *)&short_id, SHORT_ID_SIZE);
  offset += SHORT_ID_SIZE;
}


// read node from buffer
void NodeFile::deserializeNode(Node* node, ShortId& short_id, const ByteVec& buffer, unsigned& offset)
{
  memcpy((char *)node->point, (void*)&buffer[offset], POINT_SIZE);
  offset += POINT_SIZE;
  memcpy((char*)node->color, (void*)&buffer[offset], COLOR_SIZE);
  offset += COLOR_SIZE;
  memcpy((char *)&node->count, (void*)&buffer[offset], COUNT_SIZE);
  offset += COUNT_SIZE;
  memcpy((char *)&node->children, (void*)&buffer[offset], CHILDREN_SIZE);
  offset += CHILDREN_SIZE;
  memcpy((char *)&short_id, (void*)&buffer[offset], SHORT_ID_SIZE);
  offset += SHORT_ID_SIZE;
}



}
