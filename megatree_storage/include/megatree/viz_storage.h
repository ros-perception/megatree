#ifndef MEGATREE_VIZ_STORAGE_H_
#define MEGATREE_VIZ_STORAGE_H_

#include <megatree/storage.h>
#include <cstdio>

namespace megatree
{
class VizStorage : public Storage
{
public:
  VizStorage(const boost::filesystem::path &path);
  ~VizStorage() {}

  virtual void get(const boost::filesystem::path &path, ByteVec &result);
  virtual void getBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results);
  virtual void getAsync(const boost::filesystem::path &path, GetCallback callback);

  // type
  virtual std::string getType() {return std::string("VizStorage"); };

  // writing is not supported
  virtual void putBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data) 
  {printf("VizStorage does not support writing"); abort();};
  virtual void putAsync(const boost::filesystem::path &path, const ByteVec& data, PutCallback callback)
  {printf("VizStorage does not support writing"); abort();};
  

private:
  boost::filesystem::path tree;
  boost::shared_ptr<Storage> storage;
  unsigned subtree_width;

  void convert(const ByteVec& data_in, ByteVec& data_out);
  void convertCb(const boost::filesystem::path &path, GetCallback cb, const ByteVec& data);
};

}// namespace
#endif
