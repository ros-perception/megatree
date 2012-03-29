#ifndef MEGATREE_ALLOCATOR_H
#define MEGATREE_ALLOCATOR_H

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <cstdio>

namespace megatree
{


template <class T>
class Allocator
{
public:
  Allocator(unsigned size=0)
    : objects(NULL), overflow(0)
  {
    objects = new T[size];
    obj_stack.reserve(size);
    assert(objects);
    for (unsigned i=0; i<size; i++)
      obj_stack.push_back(&objects[i]);

    printf("Allocator for %d objects takes %d bytes\n", (int)size, (int)(size*sizeof(T)));
  }

  ~Allocator()
  {
    boost::mutex::scoped_lock lock(mutex);
    delete [] objects;
  }

  T* allocate()
  {
    boost::mutex::scoped_lock lock(mutex);
    assert(!obj_stack.empty());
    T* obj = obj_stack.back();
    obj_stack.pop_back();
    return obj;
  }

  void allocateMany(size_t howmany, std::vector<T*> &vec)
  {
    boost::mutex::scoped_lock lock(mutex);
    assert(obj_stack.size() >= howmany);

    typename std::vector<T*>::iterator begin = obj_stack.end() - howmany;

    // Moves begin..obj_stack.end() from obj_stack to vec
    vec.clear();
    vec.reserve(howmany);
    vec.insert(vec.begin(), begin, obj_stack.end());
    obj_stack.erase(begin, obj_stack.end());
  }


  void deAllocate(T* obj)
  {
    boost::mutex::scoped_lock lock(mutex);
    obj_stack.push_back(obj);
  }

  void deallocateMany(std::vector<T*> &vec)
  {
    boost::mutex::scoped_lock lock(mutex);
    obj_stack.insert(obj_stack.end(), vec.begin(), vec.end());
    vec.clear();
  }


private:
  boost::mutex mutex;
  T* objects;
  std::vector<T*> obj_stack;
  unsigned overflow;

};

}

#endif
