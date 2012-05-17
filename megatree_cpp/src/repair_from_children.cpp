#include <megatree/node_file.h>
#include <megatree/storage_factory.h>

int main(int argc, char** argv)
{
  using namespace megatree;
  boost::shared_ptr<Storage> storage(openStorage("hbase://wgsc2/tahoe_all"));
  std::vector<boost::shared_ptr<NodeFile> > children;

  std::string broken_file = "f1611111131";

  children.resize(8);
  for(int i = 0; i < 8; ++i)
  {
    char buf[100];
    snprintf(buf, 100, "%s%d", broken_file.c_str(), i);
    std::string child_file = buf;
    ByteVec bytes;
    storage->get(child_file, bytes);
    if(!bytes.empty())
    {
      children[i].reset(new NodeFile(child_file));
      children[i]->deserialize(bytes);
      printf("Deserialize %d with %d nodes\n", i, children[i]->cacheSize());
    }
  }

  NodeFile broken_node_file(broken_file);
  broken_node_file.initializeFromChildren(broken_file, children);
  printf("Cache size of reconstructed file %d\n", broken_node_file.cacheSize());

  ByteVec data_to_write;
  broken_node_file.serialize(data_to_write);
  printf("Size of data to write %zu\n", data_to_write.size());
  storage->put(broken_file, data_to_write);

  return 0;
}
