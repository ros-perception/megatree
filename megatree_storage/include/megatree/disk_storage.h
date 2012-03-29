#ifndef MEGATREE_DISK_STORAGE_H_
#define MEGATREE_DISK_STORAGE_H_

#include <string>
#include <iostream>
#include <fstream>
#include <boost/iostreams/device/mapped_file.hpp>

#include <megatree/storage.h>
#include <megatree/function_caller.h>

namespace megatree {

class DiskStorage : public Storage
{
public:
  DiskStorage(const boost::filesystem::path &_root) : root(_root), function_caller(5) {}
  ~DiskStorage() {}

  virtual void get(const boost::filesystem::path &path, ByteVec &result);
  virtual void getBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results);
  virtual void putBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data);
  
  virtual void getAsync(const boost::filesystem::path &path, GetCallback callback);
  virtual void putAsync(const boost::filesystem::path &path, const ByteVec& data, PutCallback callback);
  
  virtual std::string getType() {return std::string("DiskStorage"); };
private:
  boost::filesystem::path root;
  FunctionCaller function_caller;
  void readerFunction(const boost::filesystem::path &path, GetCallback callback);
  void writerFunction(const boost::filesystem::path &path, const ByteVec &data, PutCallback callback);
};




class DiskTempDir : public TempDir
{
public:
  DiskTempDir(const boost::filesystem::path &prefix = "", bool remove = true)
    : remove_(remove)
  {
    std::string tmp_storage = prefix.string() + "XXXXXX";
    char *tmp = mkdtemp(&tmp_storage[0]);
    assert(tmp);
    printf("Temporary directory: %s\n", tmp);

    path_ = tmp;
  }

  ~DiskTempDir()
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



}

#endif
