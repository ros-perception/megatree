#include <megatree/storage.h>
#include <megatree/storage_server_ros.h>
#include <megatree/disk_storage.h>
#include <megatree/compress.h>
#include <megatree/storage_factory.h>
#include <argp.h>
#include <unistd.h>
#include <iostream>
#include <argp.h>



namespace megatree 
{
  StorageServer::StorageServer(boost::shared_ptr<Storage> storage, const std::string& ns)
    : storage_(storage)
  {
    ros::NodeHandle nh;
    pub_ = nh.advertise<megatree_storage::StorageResponse>(ns+"/response", 100);
    sub_ = nh.subscribe(ns+"/request", 100, &StorageServer::requestCb, this);
    
    printf("Storage server is running in namespace %s...\n", ns.c_str());
  }

  void StorageServer::requestCb(const megatree_storage::StorageRequestConstPtr& req)
  {
    //printf("Storage server received request for path %s\n", req->path.c_str());
    storage_->getAsync(req->path, boost::bind(&StorageServer::responseCb, this, req->path, req->compression_method, _1));
  }

  void StorageServer::responseCb(const std::string& path, const uint8_t& compression_method, const ByteVec& data)
  {
    //printf("Storage server received response for path %s\n", path.c_str());

    megatree_storage::StorageResponse resp;

    // only compress node files
    if (compression_method == 0)
      resp.data = data;
    else if (compression_method == 1)
      compress(data, resp.data);
    else
    {
      printf("Unknown compression method %d\n", compression_method);
      abort();
    }

    resp.path = path;
    resp.compression_method = compression_method;
    pub_.publish(resp);
  }

}// namespace



struct arguments_t {
  char* tree;
  char* ns;
};


static int parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments_t *arguments = (arguments_t*)state->input;
  
  switch (key)
  {
  case 't':
    arguments->tree = arg;
    break;
  case 'n':
    arguments->ns = arg;
    break;
  }
  return 0;
}



int main(int argc, char** argv)
{
  ros::init(argc, argv, "storage_server");

  // Parses command line options
  struct arguments_t arguments;
  arguments.tree = 0;
  arguments.ns = 0;

  struct argp_option options[] = {
    {"tree",       't', "TREE",   0,     "Path to tree"},
    {"namespace",  'n', "NS",     0,     "Namespace of the server"},
    { 0 }
  };
  struct argp argp = { options, parse_opt };
  int parse_ok = argp_parse(&argp, argc, argv, 0, 0, &arguments);
  printf("Arguments parsed: %d\n", parse_ok);
  printf("Tree: %s\n", arguments.tree);

  if (!arguments.tree) {
    fprintf(stderr, "No tree path given. Use -t option\n");
    return 1;
  }
  if (!arguments.ns) {
    fprintf(stderr, "No server namespace given. Use -n option\n");
    return 1;
  }

  boost::shared_ptr<megatree::Storage> storage(megatree::openStorage(arguments.tree));
  megatree::StorageServer ss(storage, arguments.ns);
  
  ros::spin();
}
