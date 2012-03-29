#include <megatree/storage_factory.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <cstdio>


int main(int argc, char** argv)
{
  if (argc != 3)
  {
    printf("Usage: ./write tree file\n");
    return 0;
  }

  std::string tree_path(argv[1]);
  std::string filename(argv[2]);

  // safety
  if (filename != "metadata.ini" && filename != "views.ini" )
  {
    printf("For now write is restricted to metadata.ini\n");
    return 0;
  }
  
  // read file into bytevec
  boost::iostreams::mapped_file file(filename);
  megatree::ByteVec data(file.size());
  memcpy((void*)&data[0], file.const_data(), file.size());
  file.close();

  // write file to storage
  printf("Opening storage for writing\n");
  boost::shared_ptr<megatree::Storage> mystorage = megatree::openStorage(tree_path);
  printf("Opened storage for writing\n");
  mystorage->put(filename, data);
  printf("Wrote file from disk to hbase\n");
  

  return 0;
}
