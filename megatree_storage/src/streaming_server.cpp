#include <cstdio>
#include <deque>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <boost/asio.hpp>

#include <megatree/blocking_queue.h>

// Threads:
// A. Request reader thread (from client)
// B. Response writer thread (to client)
// C. Thrift thread

using boost::asio::ip::tcp;

typedef std::vector<uint8_t> Message;

class ClientHandler
{
public:
  ClientHandler(boost::asio::io_service &io_service) : socket(io_service)
  {
  }
  
  ~ClientHandler()
  {
    stop();
    reader_thread.join();
    writer_thread.join();
  }

  void start()
  {
    keep_running = true;
    reader_thread = boost::thread(boost::bind(&ClientHandler::readerThread, this));
    writer_thread = boost::thread(boost::bind(&ClientHandler::writerThread, this));
  }
  
  void stop()
  {
    keep_running = false;
    reader_thread.interrupt();
    writer_thread.interrupt();
  }

  void waitUntilDone()
  {
    while (keep_running)
      usleep(100);
    reader_thread.join();
    writer_thread.join();
  }

  // Next:
  // - Open up a storage (from a command line parameter, sent directly to the storage factory)
  // - Create the storage thread, that:
  //   - Pops incoming queue
  //   - Calls get (or getBatch) on storage (put???)
  //   - Pushes the results onto outgoing queue
  //
  // Later: Create StreamingStorage, which connects to the streaming server
  // Even later: Create a "storage tester" which checks that the storage works properly

  void readerThread()
  {
    while (keep_running)
    {
      // Reads the message size
      std::vector<uint8_t> header(4);
      boost::system::error_code error;
      size_t ret;
      ret = boost::asio::read(socket, boost::asio::buffer(header, 4),
                              boost::asio::transfer_all(), error);
      if (error) {
        fprintf(stderr, "Error in reader thread: %s\n", error.message().c_str());
        stop();
      }
      assert(ret == 4);
      
      // Little endian
      uint32_t message_len = (header[0]) + (header[1] << 1) + (header[2] << 2) + (header[3] << 3);
      Message message(message_len - 4);

      // Reads in the rest of the message
      ret = boost::asio::read(socket, boost::asio::buffer(message, message.size()),
                              boost::asio::transfer_all(), error);
      if (error) {
        fprintf(stderr, "Error in reader thread: %s\n", error.message().c_str());
        stop();
      }
      assert(ret == message.size());

      // Enqueues the received message
      size_t current_size = incoming_queue.enqueue(message);
      printf("Received.  Incoming queue: %zu\n", current_size);
    }
  }

  void writerThread()
  {
    while (keep_running)
    {
      size_t ret;
      Message message = outgoing_queue.dequeue();

      uint32_t len = message.size() + 4;
      std::vector<uint8_t> header(4);
      header[0] = len & 0xff;
      header[1] = (len >> 1) & 0xff;
      header[2] = (len >> 2) & 0xff;
      header[3] = (len >> 3) & 0xff;

      // Writes the header
      ret = boost::asio::write(socket, boost::asio::buffer(header, header.size()));
      assert(ret == 4);

      // Writes the message
      ret = boost::asio::write(socket, boost::asio::buffer(message, message.size()));
      assert(ret == message.size());
    }
  }

  tcp::socket socket;
  bool keep_running;
  boost::thread reader_thread, writer_thread;

  megatree::BlockingQueue<Message> incoming_queue, outgoing_queue;
};

int main(int argc, char** argv)
{
  printf("Hello world\n");

  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 22322));
    for (;;) {
      ClientHandler handler(io_service);
      //tcp::socket socket(io_service);
      acceptor.accept(handler.socket);
      printf("Accepted!\n");
      handler.start();
      handler.waitUntilDone();
    }
  }
  catch (std::exception &e)
  {
    fprintf(stderr, "Exception: %s\n", e.what());
  }
}

