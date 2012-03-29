#include <megatree/storage_client_ros.h>
#include <megatree/compress.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>


namespace megatree {



StorageClient::StorageClient(const boost::filesystem::path& server_ns_) 
  : server_ns(server_ns_.string()), running(true), initialized_(false)
{
  if (server_ns.substr(0, 6) == std::string("ros://"))
  {
    server_ns = server_ns.substr(6, server_ns.size());
  }
  printf("Storage client in namespace %s\n", server_ns.c_str());
}


StorageClient::~StorageClient()
{
  running = false;
  receive_thread_.interrupt();
  ros_thread_.interrupt();


  receive_thread_.join();
  ros_thread_.join();
}

void StorageClient::initialize()
{
  // connections
  ros::NodeHandle nh;
  pub_ = nh.advertise<megatree_storage::StorageRequest>(server_ns+"/request", 100);
  sub_ = nh.subscribe(server_ns+"/response", 100, &StorageClient::responseCb, this);

  // wait until connected
  while (ros::ok() && (pub_.getNumSubscribers() < 1 || sub_.getNumPublishers() < 1))
  {
    printf("Waiting for storage_server in namespace %s...\n", server_ns.c_str());
    ros::Duration(1.0).sleep();
  }
  printf("Connected to storage_server.\n");

  // start ros thread
  receive_thread_ = boost::thread(boost::bind(&StorageClient::receiveThread, this));
  ros_thread_ = boost::thread(boost::bind(&ros::spin));

  initialized_ = true;
  ros::Duration(2.0).sleep();
}




void StorageClient::getAsync(const boost::filesystem::path &path, GetCallback callback)
{
  boost::mutex::scoped_lock lock(request_buffer_mutex);

  assert(request_buffer.find(path) == request_buffer.end());
  request_buffer.insert(std::pair<boost::filesystem::path, GetCallback>(path, callback));

  sendRequest(path);
}


void StorageClient::sendRequest(const boost::filesystem::path& path)
{
  if (!initialized_)
    initialize();

  megatree_storage::StorageRequest req;
  req.path = path.string();
  if (req.path == "metadata.ini" || req.path == "f")
    req.compression_method = 0;  // no compression
  else
    req.compression_method = 1;
  pub_.publish(req);
  //printf("Storage client sent request for %s\n", path.string().c_str());
}


void StorageClient::receiveThread()
{
  boost::mutex::scoped_lock lock(response_buffer_mutex);
  while (running)
  {
  
    // process entire buffer
    while (running && !response_buffer.empty())
    {
      //printf("Storage client processes response from storage server\n");
    
      boost::mutex::scoped_lock lock(request_buffer_mutex);      

      // call callback 
      std::map<boost::filesystem::path, GetCallback>::iterator it = request_buffer.find(response_buffer.front()->path);
      if (it == request_buffer.end())
      {
        printf("Could not find path %s in request buffer\n", response_buffer.front()->path.c_str());
        assert(it != request_buffer.end());
      }

      // uncompress node files
      if (response_buffer.front()->compression_method == 0)
      {
        it->second(response_buffer.front()->data); // no compression
      }
      else if (response_buffer.front()->compression_method == 1)
      {
        ByteVec uncompressed_data;
        extract(response_buffer.front()->data, uncompressed_data);
        /*
        std::cout << "uncompressed data : ";
        for (unsigned i=0; i<uncompressed_data.size(); i++)
          std::cout << (unsigned)(uncompressed_data[i]) << " - ";
        std::cout << std::endl;
        */
        it->second(uncompressed_data);
      }
      else
      {
        printf("Unknown compression method %d\n", response_buffer.front()->compression_method);
        abort();
      }
      printf("Retrieved node file %s\n", response_buffer.front()->path.c_str());

      // erase from both buffers
      request_buffer.erase(it);
      response_buffer.pop_front();
    }
    response_buffer_condition.wait(lock);
  }
}


void StorageClient::responseCb(const megatree_storage::StorageResponseConstPtr& msg)
{
  //printf("Storage client received response from storage server\n");

  boost::mutex::scoped_lock lock(response_buffer_mutex);
  response_buffer.push_back(msg);
  response_buffer_condition.notify_all();
}

}
