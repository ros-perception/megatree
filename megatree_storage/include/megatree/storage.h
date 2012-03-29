#ifndef MEGATREE_STORAGE_H_
#define MEGATREE_STORAGE_H_

#include <assert.h>
#include <megatree/tree_common.h>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/bind.hpp>

namespace megatree {



class Storage
{
public:
  virtual ~Storage() {}

  virtual void get(const boost::filesystem::path &path, ByteVec &result)
  {
    std::vector<boost::filesystem::path> p;
    std::vector<ByteVec> r;
    p.push_back(path);
    getBatch(p, r);

    if(r.empty())
      return;

    assert(r.size() == 1);
    result.swap(r[0]);
  }

  virtual void getBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results)
  {
    results.resize(paths.size());
    unsigned remaining = paths.size();
    boost::condition get_condition;

    // Async call to retrieve data
    for (size_t i = 0; i < paths.size(); ++i)
      getAsync(paths[i], boost::bind(&Storage::getDataCb, this, boost::ref(get_condition), boost::ref(remaining), _1, boost::ref(results[i])));

    // Wait for all data to arrive
    boost::mutex get_mutex;
    boost::mutex::scoped_lock lock(get_mutex);
    get_condition.wait(get_mutex);
  }

  virtual void put(const boost::filesystem::path &path, const ByteVec &data)
  {
    std::vector<boost::filesystem::path> p;
    std::vector<ByteVec> d;
    p.push_back(path);
    d.push_back(data);
    putBatch(p, d);
  }

  virtual void putBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data)
  {
    assert(paths.size() == data.size());
    unsigned remaining = paths.size();
    boost::condition put_condition;

    // Async call to send data
    for (size_t i = 0; i < paths.size(); ++i)
      putAsync(paths[i], data[i], boost::bind(&Storage::putDataCb, this, boost::ref(put_condition), boost::ref(remaining)));

    // Wait for all data to be sent
    boost::mutex put_mutex;
    boost::mutex::scoped_lock lock(put_mutex);
    put_condition.wait(put_mutex);
  }


  typedef boost::function<void(const ByteVec&)> GetCallback;
  virtual void getAsync(const boost::filesystem::path &path, GetCallback callback) = 0;


  typedef boost::function<void(void)> PutCallback;
  virtual void putAsync(const boost::filesystem::path &path, const ByteVec &data, PutCallback callback) = 0;
  
  virtual std::string getType() = 0;

protected:
  Storage() {}
  
private:
  void getDataCb(boost::condition& get_condition, unsigned& remaining, const ByteVec& data_in, ByteVec& data)
  {
    remaining--;
    data = data_in;
    
    if (remaining == 0)
      get_condition.notify_one();
  }

  void putDataCb(boost::condition& put_condition, unsigned& remaining)
  {
    remaining--;
    
    if (remaining == 0)
      put_condition.notify_one();
  }
};


class TempDir
{
protected:
  TempDir() {}
public:
  virtual ~TempDir() {}
  virtual const boost::filesystem::path &getPath() const = 0;
};


}


#endif
