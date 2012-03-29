#ifndef MEGATREE_STORAGE_SERVER_H_
#define MEGATREE_STORAGE_SERVER_H_

#include <ros/ros.h>
#include <megatree_storage/StorageResponse.h>
#include <megatree_storage/StorageRequest.h>
#include <megatree/storage.h>


namespace megatree
{
class StorageServer
{
public:
  StorageServer(boost::shared_ptr<Storage> storage, const std::string& ns);
  ~StorageServer() {};

private:
  void requestCb(const megatree_storage::StorageRequestConstPtr& req);
  void responseCb(const std::string& path, const uint8_t& compression_method, const ByteVec& data);

  ros::Publisher pub_;
  ros::Subscriber sub_;
  boost::shared_ptr<Storage> storage_;

};

}// namespace
#endif
