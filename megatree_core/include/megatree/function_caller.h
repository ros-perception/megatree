#ifndef MEGATREE_FUNCTION_CALLER_H
#define MEGATREE_FUNCTION_CALLER_H


#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <vector>

namespace megatree
{

  class FunctionCaller
  {
  public:
    FunctionCaller(unsigned thread_pool_size=1)
      :running(true)
    {
      threads.resize(thread_pool_size);
      for (unsigned i=0; i<threads.size(); i++)
        threads[i] = new boost::thread(boost::bind(&FunctionCaller::threadLoop, this, i));
    }


    ~FunctionCaller()
    {
      // notify all threads that are locked on condition variable
      {
        boost::mutex::scoped_lock lock(mutex);
        running = false;
        condition.notify_all();
      }

      for (unsigned i=0; i<threads.size(); i++)
      {
        threads[i]->join();
        delete threads[i];
      }
    }



   void addFunction(boost::function<void(void)> func)
    {
      boost::mutex::scoped_lock lock(mutex);
      queue.push_front(func);
      //printf("FunctionCaller ADD    ==> %5zu\n", queue.size());
      condition.notify_one();
    }


    void threadLoop(unsigned i)
    {
      while (true)
      {
        boost::function<void(void)> f;
        {
          // wait for condition if queue is empty
          boost::mutex::scoped_lock lock(mutex);  
          while (queue.empty())
          {
            if (!running)
              return;
            condition.wait(lock);
          }
          if (!running)
            return;

          assert(!queue.empty());
          f = queue.back();
          queue.pop_back();
          //printf("FunctionCaller REMOVE ==> %5zu\n", queue.size());
        }
        // call actual function
        f();
      }
    }


  private:
    boost::condition condition;

    bool running;
    std::list<boost::function<void(void)> > queue;
    boost::mutex mutex;
    std::vector<boost::thread*> threads;
  };

}
#endif
