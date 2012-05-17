#include <megatree/common.h>
#include <megatree/megatree.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>

#include <unistd.h>
#include <iostream>

using namespace megatree;

class TempDirectory
{
public:
  TempDirectory(const std::string &prefix = "", bool remove = true)
    : remove_(remove)
  {
    std::string tmp_storage = prefix + "XXXXXX";
    char *tmp = mkdtemp(&tmp_storage[0]);
    assert(tmp);
    printf("Temporary directory: %s\n", tmp);

    path_ = tmp;
  }

  ~TempDirectory()
  {
    if (remove_)
      boost::filesystem::remove_all(path_);
  }

  const boost::filesystem::path &getPath() const
  {
    return path_;
  }

private:
  boost::filesystem::path path_;
  bool remove_;
};


int main (int argc, char** argv)
{
  // create megatree
  std::vector<double> tree_center(3, 0);
  double tree_size = 2 * (6378000+8850+10000); // radius of the earth + height of Mount Everest + padding

  if(argc != 4)
  {
    printf("Usage: ./create tree_path  subtree_width subfolder_depth\n");
    return -1;
  }

  boost::filesystem::path tree_path(argv[1]);
  removePath(tree_path);
  boost::shared_ptr<Storage> storage(openStorage(tree_path));
  MegaTree tree(storage, tree_center, tree_size,
                atoi(argv[2]), atoi(argv[3]),  // subtree_width, subfolder_depth
                1000000);
  
  return 0;
}
