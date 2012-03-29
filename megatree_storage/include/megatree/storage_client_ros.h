#ifndef MEGATREE_STORAGE_CLIENT_H_
#define MEGATREE_STORAGE_CLIENT_H_

#include <string>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <map>
#include <list>
#include <megatree/storage.h>
#include <megatree/function_caller.h>
#include <ros/ros.h>
#include <megatree_storage/StorageResponse.h>
#include <megatree_storage/StorageRequest.h>

namespace megatree {



class StorageClient : public Storage
{
public:
  StorageClient(const boost::filesystem::path &server_ns_="storage_server");
  ~StorageClient();

  virtual void getAsync(const boost::filesystem::path &path, GetCallback callback);

  virtual void putAsync(const boost::filesystem::path &path, const ByteVec& data, PutCallback callback)
  {
    printf("putAsync not implemented for storage client\n");
    abort();
  }
  
  virtual std::string getType() {return std::string("ClientStorage"); };
private:
  void initialize();
  void sendRequest(const boost::filesystem::path& path);
  void receiveThread();

  void responseCb(const megatree_storage::StorageResponseConstPtr& msg);

  std::string server_ns;

  bool running;
  boost::mutex response_buffer_mutex, request_buffer_mutex;
  boost::condition response_buffer_condition;
  std::list<megatree_storage::StorageResponseConstPtr> response_buffer;
  std::map<boost::filesystem::path, GetCallback> request_buffer;

  bool initialized_;
  ros::Publisher pub_;
  ros::Subscriber sub_;

  boost::thread receive_thread_, ros_thread_;
};



}

#endif
