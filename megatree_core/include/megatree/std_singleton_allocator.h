#ifndef MEGATREE_STD_SINGLETON_ALLOCATOR
#define MEGATREE_STD_SINGLETON_ALLOCATOR

#include <cstdio>
#include <boost/thread/mutex.hpp>
#include <typeinfo>
#include <vector>

namespace megatree
{

template <class T>
class StdSingletonAllocatorInstance
{
 public:
  static StdSingletonAllocatorInstance<T>* getInstance(size_t size=0)
  {
    if (instance_ == NULL)
    {
      printf("first time construction of instance because instance pointer is %p\n", instance_);
      T t;
      //printf("Template Type: %s\n", typeid(t).name());
      assert(size != 0);
      instance_ = new StdSingletonAllocatorInstance<T>(size);
    }
    /*
    else
      printf("Get another instance of the allocator\n");
    */
    return instance_;
  }
  
  size_t max_size() const
  {
    return size;
  }

  T* allocate(size_t n)
  {
    assert(n == 1);
    boost::mutex::scoped_lock lock(mutex);
    T* obj = mem_vec.back();
    mem_vec.pop_back();
    return obj;
  }

  void deallocate(T* obj, size_t n)
  {
    assert(n == 1);
    boost::mutex::scoped_lock lock(mutex);
    mem_vec.push_back(obj);
  }  

  void destruct()
  {
    printf("Free all memory\n");
    delete[] mem;
  }


 private:
  StdSingletonAllocatorInstance(size_t size_)  
    :size(size_)
  {
    mem = new T[size];
    mem_vec.resize(size);
    for (size_t i=0; i<size; i++)
      mem_vec[i] = &mem[i];
    printf("Constructed StdSingletonAllocatorInstance with size %zu\n", size);
  }

  static StdSingletonAllocatorInstance<T>* instance_;

  size_t size;
  T* mem;
  std::vector<T*> mem_vec;

  boost::mutex mutex;

};








template <class T>
class StdSingletonAllocator
{
 public:

  // required typedefs
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;
  template <class U> struct rebind { typedef StdSingletonAllocator<U> other; };

  // constructor gets singleton
  StdSingletonAllocator(): allocator_(StdSingletonAllocatorInstance<T>::getInstance()) {};
  
  // copy constructor
  StdSingletonAllocator(const StdSingletonAllocator<T>& a) 
  {
    allocator_ = a.allocator_;
    //printf("Copy constructor of StdSingletonAllocator with instance pointer %p\n", allocator_);
  };

  template <class U> StdSingletonAllocator(const StdSingletonAllocator<U>&)
  {
    //printf("Other copy constructor\n");
  }

  // destructor
  ~StdSingletonAllocator() {};

  // address of object
  pointer address(reference v) const
  {
    return &v;
  }

  // address of const object
  const_pointer addres(const_reference v) const
  {
    return &v;
  }

  // max size
  size_t max_size() const
  {
    //printf("Max size\n");
    return allocator_->max_size();
  }

  // actuall allocation
  pointer allocate(size_type n)
  {
    //printf("Allocate\n");
    return allocator_->allocate(n);
  }

  void deallocate(pointer obj, size_type n)
  {
    //printf("Deallocate\n");
    allocator_->deallocate(obj, n);
  }

  void construct(pointer p, const_reference v) const
  {
    //printf("Construct\n");
    new (static_cast<void*>(p)) T(v);
  }

  void destroy(pointer p) const
  {
    //printf("Destroy\n");
  }


 private:
  StdSingletonAllocatorInstance<T>* allocator_;
};


template <class T1, class T2>
bool operator ==(const StdSingletonAllocator<T1>& a1, const StdSingletonAllocator<T2>& a2)
{
  //printf("Operator ==\n");
  return true;
}


template <class T1, class T2>
bool operator !=(const StdSingletonAllocator<T1>& a1, const StdSingletonAllocator<T2>& a2)
{
  //printf("Operator !=\n");
  return false;
}


template <class T>
StdSingletonAllocatorInstance<T>* StdSingletonAllocatorInstance<T>::instance_ = NULL;


 /*
namespace std
{
  template <class T1, class T2>
  StdSingletonAllocator<T2>& __stl_alloc_rebind(StdSingletonAllocator<T1>& a, const T2*) 
  {  
    printf("std_alloc_rebind\n");
    return (StdSingletonAllocator<T2>&)(a); 
  }
  

  template <class T1, class T2>
  StdSingletonAllocator<T2> __stl_alloc_create(const StdSingletonAllocator<T1>&, const T2*) 
  { 
    printf("std_alloc_create\n");
    return StdSingletonAllocator<T2>(); 
  }
  
} // namespace std
 */


} // namespace

#endif
