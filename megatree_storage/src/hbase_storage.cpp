#include <megatree/hbase_storage.h>

#include <algorithm>

#define USE_FRAMED_TRANSPORT 0

namespace megatree {

using namespace apache::hadoop::hbase::thrift;

const static size_t ASYNC_READ_BATCH_SIZE = 10000;
//const static size_t ASYNC_READ_BATCH_SIZE = 100;
const static size_t ASYNC_WRITE_BATCH_SIZE = 100;
const static unsigned int NUM_READER_THREADS = 10;
const static unsigned int NUM_WRITER_THREADS = 1;

//static size_t nodefiles_touched = 0;

void parseHbasePath(const boost::filesystem::path &path_,
                    std::string &server, unsigned int &port, std::string &table)
{
  std::string path = path_.string();
  assert(path.substr(0, 8) == std::string("hbase://"));

  // Separates off the server&port
  size_t serverport_end = path.find('/', 8);
  std::string serverport = path.substr(8, serverport_end - 8);

  size_t server_end = serverport.find(':', 0);
  if (server_end == std::string::npos) {
    server = serverport;
    port = 9090;
  }
  else {
    server = serverport.substr(0, server_end);
    std::string port_str = serverport.substr(server_end + 1);
    port = atoi(port_str.c_str());
  }

  // The rest is the table name.
  table = path.substr(serverport_end + 1);
  while (table[table.size() - 1] == '/')
    table.resize(table.size() - 1);
}

HbaseStorage::HbaseStorage(const boost::filesystem::path &_root)
  : async_thread_keep_running(true)
{
  using namespace apache::thrift::transport;
  using namespace apache::thrift::protocol;

  parseHbasePath(_root, server, port, table);
  printf("Connecting to hbase %s:%u, table %s\n", server.c_str(), port, table.c_str());

  // Connects to Hbase.
  socket.reset(new TSocket(server, port));
#if USE_FRAMED_TRANSPORT
  transport.reset(new TFramedTransport(socket));
#else
  transport.reset(new TBufferedTransport(socket));
#endif
  protocol.reset(new TBinaryProtocol(transport));
  client.reset(new HbaseClient(protocol));
  transport->open();

  // Checks if the table already exists.
  std::vector<std::string> table_names;
  client->getTableNames(table_names);
  std::vector<std::string>::iterator found = std::find(table_names.begin(), table_names.end(), table);
  if (found == table_names.end()) {
    // Creates the table
    std::vector<ColumnDescriptor> column_families(1);
    column_families[0].name = "data";
    client->createTable(table, column_families);
  }

  // Starts the async read/write threads
  async_thread_keep_running = true;

  async_threads.resize(NUM_READER_THREADS + NUM_WRITER_THREADS);
  for(size_t i = 0; i < NUM_READER_THREADS; ++i)
    async_threads[i].reset(new boost::thread(boost::bind(&HbaseStorage::asyncReadThread, this)));

  for(size_t i = 0; i < NUM_WRITER_THREADS; ++i)
    async_threads[i + NUM_READER_THREADS].reset(new boost::thread(boost::bind(&HbaseStorage::asyncWriteThread, this)));
}

HbaseStorage::~HbaseStorage()
{
  // Stops the async thread
  async_thread_keep_running = false;
  write_queue_cond.notify_all();
  read_queue_cond.notify_all();

  // boost::thread has a bug in interrupt: interrupt does not always
  // affect conditions.  The workaround is to hold onto the mutex
  // while interrupting.
  //
  // See: https://svn.boost.org/trac/boost/ticket/2330
  // Workaround: https://svn.boost.org/trac/boost/ticket/3735
  {
    // TODO: Locking these mutices together is bad.  Should separate
    // threads into a thread group of readers and a thread group of
    // writers.
    boost::mutex::scoped_lock writer_lock(write_queue_mutex);
    boost::mutex::scoped_lock reader_lock(read_queue_mutex);
    for(size_t i = 0; i < async_threads.size(); ++i) {
      async_threads[i]->interrupt();
    }
  }
  for(size_t i = 0; i < async_threads.size(); ++i) {
    if (!async_threads[i]->timed_join(boost::posix_time::seconds(10))) {
      fprintf(stderr, "Difficulty joining boost thread %zu during HbaseStorage destruction\n", i);
    }
    async_threads[i]->join();
  }
  
  transport->close();
}


void HbaseStorage::getBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results)
{
  boost::mutex::scoped_lock lock(socket_mutex);
  getBatch(client, paths, results);
}

void HbaseStorage::getBatch(boost::shared_ptr<apache::hadoop::hbase::thrift::HbaseClient> passed_client,
                            const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results)
{
  results.resize(paths.size());

  std::vector<std::string> rows(paths.size());
  for (size_t i = 0; i < paths.size(); ++i)
    rows[i] = paths[i].string();

  // Gets the row
  std::vector<TRowResult> row_results;
  //boost::posix_time::ptime started = boost::posix_time::microsec_clock::universal_time();
  //printf("Making a get batch request\n");
  passed_client->getRows(row_results, table, rows);
  //boost::posix_time::ptime finished = boost::posix_time::microsec_clock::universal_time();
  //nodefiles_touched += paths.size();
  //printf("Get batch for size %zu, finished in %.4f seconds, nodefiles touched %zu\n", paths.size(), (finished - started).total_milliseconds() / 1000.0f, nodefiles_touched);
  if (row_results.empty()) {
    fprintf(stderr, "getBatch failed.  Not sure what to do here\n");
    results.clear();
    return;
  }
  if (row_results.size() != rows.size())
  {
    fprintf(stderr, "Requested %zu rows, but received %zu\n", rows.size(), row_results.size());
    fprintf(stderr, "Rows requested:");
    for (size_t i = 0; i < rows.size(); ++i)
      fprintf(stderr, " %s", rows[i].c_str());
    fprintf(stderr, "\n");
    abort();
  }

  // Gets the file data from each row result.
  for (size_t i = 0; i < row_results.size(); ++i)
  {
    // Finds the data: column
    std::map<std::string, TCell>::iterator it = row_results[i].columns.find("data:");
    if (it == row_results[i].columns.end()) {
      fprintf(stderr, "Results for row %s were not empty (%zu), but column \"data:\" was not found!\n",
              paths[i].string().c_str(), results[i].size());
      abort();
    }

    results[i].resize(it->second.value.size());
    memcpy(&(results[i][0]), &(it->second.value[0]), it->second.value.size());  // TODO: Removing this copy would be nice
  }
}

void HbaseStorage::putBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data)
{
  boost::mutex::scoped_lock lock(socket_mutex);
  putBatch(client, paths, data);
}

void HbaseStorage::putBatch(boost::shared_ptr<apache::hadoop::hbase::thrift::HbaseClient> passed_client,
    const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data)
{
  assert(paths.size() == data.size());

  std::vector<BatchMutation> bm(paths.size());
  for (size_t i = 0; i < paths.size(); ++i)
  {
    bm[i].row = paths[i].string();
    bm[i].mutations.resize(1);
    bm[i].mutations[0].column = "data:";
    bm[i].mutations[0].value.resize(data[i].size());
    memcpy(&(bm[i].mutations[0].value[0]), &(data[i][0]), data[i].size()); 
  }
  
  passed_client->mutateRows(table, bm);
}


void HbaseStorage::getAsync(const boost::filesystem::path &path, GetCallback callback)
{
  // TODO: consider making this block when the read queue fills up
  boost::mutex::scoped_lock lock(read_queue_mutex);
  read_queue.push_back(ReadData(path, callback));
  read_queue_cond.notify_one();
}

void HbaseStorage::putAsync(const boost::filesystem::path &path, const ByteVec& data, PutCallback callback)
{
  boost::mutex::scoped_lock lock(write_queue_mutex);
  write_queue.push_back(WriteData(path, data, callback));
  write_queue_cond.notify_one();
}


void HbaseStorage::asyncReadThread()
{
  boost::shared_ptr<apache::thrift::transport::TSocket> thread_socket;
  boost::shared_ptr<apache::thrift::transport::TTransport> thread_transport;
  boost::shared_ptr<apache::thrift::protocol::TProtocol> thread_protocol;
  boost::shared_ptr<apache::hadoop::hbase::thrift::HbaseClient> thread_client;

  using namespace apache::thrift::transport;
  using namespace apache::thrift::protocol;

  // Connects to Hbase.
  thread_socket.reset(new TSocket(server, port));
#if USE_FRAMED_TRANSPORT
  thread_transport.reset(new TFramedTransport(thread_socket));
#else
  thread_transport.reset(new TBufferedTransport(thread_socket));
#endif
  thread_protocol.reset(new TBinaryProtocol(thread_transport));
  thread_client.reset(new HbaseClient(thread_protocol));
  thread_transport->open();

  while (async_thread_keep_running)
  {
    std::vector<boost::filesystem::path> paths;
    std::vector<GetCallback> callbacks;
    
    {
      // Waits for the read queue to have something.
      boost::mutex::scoped_lock lock(read_queue_mutex);
      while (read_queue.empty())
        read_queue_cond.wait(lock);

      // Removes a batch of read requests from the queue
      for (size_t i = 0; i < ASYNC_READ_BATCH_SIZE && !read_queue.empty(); ++i)
      {
        paths.push_back(read_queue.front().path);
        callbacks.push_back(read_queue.front().callback);
        read_queue.pop_front();
      }
    }

    // Submits the batch of read requests
    std::vector<ByteVec> results;
    getBatch(thread_client, paths, results);

    // Triggers the callbacks
    for (size_t i = 0; i < results.size(); ++i)
    {
      callbacks[i](results[i]);
    }
  }
}


void HbaseStorage::asyncWriteThread()
{
  while (async_thread_keep_running)
  {
    std::vector<boost::filesystem::path> paths;
    std::vector<ByteVec> data;
    std::vector<PutCallback> callbacks;
    
    {
      // Waits for the write queue to have something.
      boost::mutex::scoped_lock lock(write_queue_mutex);
      while (write_queue.empty())
        write_queue_cond.wait(lock);

      // Removes a batch of write requests from the queue
      for (size_t i = 0; i < ASYNC_WRITE_BATCH_SIZE && !write_queue.empty(); ++i)
      {
        paths.push_back(write_queue.front().path);
        data.push_back(write_queue.front().data);
        callbacks.push_back(write_queue.front().callback);
        write_queue.pop_front();
      }
    }

    // Submits the batch of write requests
    putBatch(paths, data);

    // Triggers the callbacks
    for (size_t i = 0; i < callbacks.size(); ++i)
    {
      callbacks[i]();
    }
  }
}


bool HbaseStorage::readFile(const std::string& row, std::string& buffer)
{
  boost::mutex::scoped_lock lock(socket_mutex);
  
  // Attempts to get the row.
  std::vector<TRowResult> results;
  client->getRow(results, table, row);

  if (results.size() == 0)
    return false;

  // Gets the file data from the row query.
  std::map<std::string, TCell>::iterator it = results[0].columns.find("data:");
  if (it == results[0].columns.end()) {
    fprintf(stderr, "Results for row %s were not empty (%zu), but column \"data:\" was not found!\n",
            row.c_str(), results.size());
    abort();
  }
  std::string &row_data = it->second.value;
  buffer.swap(row_data);  // Does this cause a copy or not?
  return true;
}

void HbaseStorage::writeFile(const std::string& row, const std::vector<unsigned char>& buffer)
{
  boost::mutex::scoped_lock lock(socket_mutex);

  std::vector<Mutation> m(1);
  m[0].column = "data:";
  m[0].value.resize(buffer.size());
  memcpy(&(m[0].value[0]), &buffer[0], buffer.size());

  client->mutateRow(table, row, m);
}




void removeHbasePath(const boost::filesystem::path &path)
{
  using namespace apache::thrift::transport;
  using namespace apache::thrift::protocol;

  // Parses the path.
  std::string server;
  unsigned int port;
  std::string table;
  parseHbasePath(path, server, port, table);

  // Connects to Hbase
  boost::shared_ptr<TSocket> socket(new TSocket(server, port));
#if USE_FRAMED_TRANSPORT
  boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
#else
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
#endif
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  apache::hadoop::hbase::thrift::HbaseClient client(protocol);
  transport->open();

  // Destroys the table
  try {
    client.disableTable(table);
    client.deleteTable(table);
  }
  catch (const apache::hadoop::hbase::thrift::IOError &ex) {
    // The table already didn't exist.
    fprintf(stderr, "Warning: Exception while deleting table %s: %s\n", table.c_str(), ex.what());
  }
  transport->close();
}

}
