#ifndef MEGATREE_HBASE_STORAGE_H_
#define MEGATREE_HBASE_STORAGE_H_

#include <string>
#include <iostream>
#include <fstream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <megatree/storage.h>

// Thrift includes.  It may be beneficial to get these out of the header eventually
#include <stdint.h>
#include <Hbase.h>
#include <transport/TSocket.h>
#include <transport/TBufferTransports.h>
#include <protocol/TBinaryProtocol.h>


namespace megatree {

// TODO: There will be big problems a an HbaseFile outlives the HbaseStorage that it was created from!


/*
 * Path structure:  hbase://server:port/table/row/morerow/stillrow
 * (The rest of the path corresponds to the row).
 * The file data is stored in the "data:" column.
 */
class HbaseStorage : public Storage
{
public:
  HbaseStorage(const boost::filesystem::path &_root);
  virtual ~HbaseStorage();
  
  virtual void getBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results);
  virtual void putBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data);

  virtual void getAsync(const boost::filesystem::path &path, GetCallback callback);
  virtual void putAsync(const boost::filesystem::path &path, const ByteVec& data, PutCallback callback);
  
  virtual std::string getType() {return std::string("HBaseStorage"); };

private:
  virtual void getBatch(boost::shared_ptr<apache::hadoop::hbase::thrift::HbaseClient> passed_client,
                        const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results);

  virtual void putBatch(boost::shared_ptr<apache::hadoop::hbase::thrift::HbaseClient> passed_client, 
                        const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data);
  

  // Thrift/HBase connection
  boost::mutex socket_mutex;
  boost::shared_ptr<apache::thrift::transport::TSocket> socket;
  boost::shared_ptr<apache::thrift::transport::TTransport> transport;
  boost::shared_ptr<apache::thrift::protocol::TProtocol> protocol;
  boost::shared_ptr<apache::hadoop::hbase::thrift::HbaseClient> client;
  std::string table;

  std::string server;
  unsigned int port;

  // Asynchronous thread
  bool async_thread_keep_running;
  std::vector<boost::shared_ptr<boost::thread> > async_threads;
  void asyncReadThread();
  void asyncWriteThread();
  
  // Asynchronous reads
  struct ReadData
  {
    ReadData(const boost::filesystem::path& path_, const GetCallback& callback_)
      : path(path_), callback(callback_){}
    boost::filesystem::path path;
    GetCallback callback;
  };
  typedef std::deque<ReadData> ReadQueue;
  ReadQueue read_queue;  // Pop off the front
  boost::mutex read_queue_mutex;
  boost::condition read_queue_cond;

  // Asynchronous writes
  struct WriteData
  {
    WriteData(const boost::filesystem::path& path_, const ByteVec& data_, const PutCallback& callback_)
      : path(path_), data(data_), callback(callback_){}

    boost::filesystem::path path;
    ByteVec data;
    PutCallback callback;
  };
  typedef std::deque<WriteData> WriteQueue;
  WriteQueue write_queue;  // Pop off the front
  boost::mutex write_queue_mutex;
  boost::condition write_queue_cond;

  void writeFile(const std::string& row, const std::vector<unsigned char>& buffer);
  bool readFile(const std::string& row, std::string& buffer);
};



void removeHbasePath(const boost::filesystem::path &path);


class HbaseTempDir : public TempDir
{
public:
  HbaseTempDir(const boost::filesystem::path &prefix = "", bool remove = true)
    : remove_(remove)
  {
    abort();
    std::string tmp_storage = prefix.string() + "XXXXXX";
    char *tmp = mkdtemp(&tmp_storage[0]);
    assert(tmp);
    printf("Temporary directory: %s\n", tmp);

    path_ = tmp;
  }

  ~HbaseTempDir()
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
