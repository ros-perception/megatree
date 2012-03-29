#ifndef MEGATREE_BLOCKING_QUEUE
#define MEGATREE_BLOCKING_QUEUE

#include <deque>
#include <boost/thread/mutex.hpp>

namespace megatree
{

static size_t WRITE_QUEUE_SIZE = 1000;

//This classs provides a thread safe implementation of a queue. It also
//enforces a max size, so calls to enqueue will block when this size is reached
//until dequeue is called.
template <class T>
class BlockingQueue
{
  public:
    BlockingQueue(size_t max_size = WRITE_QUEUE_SIZE) 
      : max_size_(max_size)
    {
    }

    //Enqueue will block on a call to enqueue when the maximum size of the
    //queue is reached. Otherwise, it just locks the queue and pushes something
    //back onto it
    size_t enqueue(const T& item)
    {
      boost::mutex::scoped_lock lock(mutex_);
      if (max_size_ > 0)
      {
        // Waits for the queue to drop below the maximum size
        while (queue_.size() >= max_size_)
          enqueue_condition.wait(lock);
      }

      //add the item onto the end of the queue
      queue_.push_back(item);

      //notify any calls to dequeue that are blocked waiting for input
      dequeue_condition.notify_one();
      return queue_.size();
    }

    //Dequeue will block on a call to dequeue when the queue is empty.
    //Otherwise, it just locks the queue and pops something from it
    T dequeue()
    {
      boost::mutex::scoped_lock lock(mutex_);

      //if there's nothing in the queue, we'll wait for something to be added
      if(queue_.empty())
        dequeue_condition.wait(lock);

      //there's something on the queue, we need to pop it off
      T item =  queue_.front();
      queue_.pop_front();

      //make sure to notify any waiting calls to enqueue that something has
      //been removed from the queue
      enqueue_condition.notify_one();

      return item;
    }

    void dequeueBatch(size_t batch_size, std::vector<T>& batch)
    {
      batch.clear();
      boost::mutex::scoped_lock lock(mutex_);

      //it there's nothing in the queue, we'll wait for something to be added
      if(queue_.empty())
        dequeue_condition.wait(lock);

      //we want to pull at most batch_size items from the queue
      while(batch.size() < batch_size && !queue_.empty())
      {
        batch.push_back(queue_.front());
        queue_.pop_front();

        //make sure to notify any waiting calles to enqueue that something has
        //been removed from the queue
        enqueue_condition.notify_one();
      }
    }

    void unblockAll()
    {
      enqueue_condition.notify_all();
      dequeue_condition.notify_all();
    }

  private:
    size_t max_size_;
    std::deque<T> queue_;
    boost::mutex mutex_;
    boost::condition enqueue_condition, dequeue_condition;
};

}

#endif
