#include <megatree/storage_factory.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <cstdio>


int main(int argc, char** argv)
{
  if (argc != 3)
  {
    printf("Usage: ./read tree file\n");
    return 0;
  }

  std::string tree_path(argv[1]);
  std::string filename(argv[2]);
  
  // read file from storage
  printf("reading data from storage\n");
  megatree::ByteVec data;
  boost::shared_ptr<megatree::Storage> mystorage = megatree::openStorage(tree_path);
  mystorage->get(filename, data);

  // write bytevec into file 
  printf("writing data to file\n");
  boost::iostreams::mapped_file_params params;
  params.path = filename;
  params.mode = std::ios_base::out;
  params.offset = 0;
  params.new_file_size = data.size();
  boost::iostreams::mapped_file file(params);
  memcpy(file.data(), (void*)&data[0], data.size());
  file.close();

  return 0;
}
