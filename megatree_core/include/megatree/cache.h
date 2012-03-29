#ifndef MEGATREE_CACHE_H
#define MEGATREE_CACHE_H

#include <boost/shared_ptr.hpp>
#include <list>
#include <tr1/unordered_map>
#include <megatree/list.h>

namespace megatree
{

template <class K, class T>
class CacheIterator;


template <class K, class T>
class Cache
{
public:
  //we need to make sure that the cache iterator can access our private Storage
  //class
  friend class CacheIterator<K, T>;

  class Storage
  {
    public:
      Storage(const K& _id, T* _object):
        id(_id), object(_object)
      {}

      K id;
      T* object;
  };

  typedef List<Storage > ObjList;
  typedef ListIterator<Storage> ObjListIterator;
  typedef std::tr1::unordered_map<K, ObjListIterator> ObjMap;
  typedef typename ObjMap::iterator ObjMapIterator;

  typedef CacheIterator<K, T> iterator;


  bool exists(const K& id)
  {
    ObjMapIterator hash_it = hash_.find(id);
    if (hash_it != hash_.end())
    {
      return true;
    }
    return false;
  }

  iterator iterate()
  {
    return CacheIterator<K, T>(this, list_.frontIterator());
  }
  iterator iterateBack()
  {
    return CacheIterator<K, T>(this, list_.backIterator());
  }

  void move_back_to_front()
  {
    list_.moveToFront(list_.getBackPointer());
  }


  void move_to_front(const K& id)
  {
    // try to find node in buffer
    ObjMapIterator hash_it = hash_.find(id);
    if (hash_it != hash_.end())
      list_.moveToFront(hash_it->second.getNode());
  }

  void move_to_front(iterator &it)
  {
    list_.moveToFront(it.getNode());
  }


  void move_to_back(const K& id)
  {
    // try to find node in buffer
    ObjMapIterator hash_it = hash_.find(id);
    if (hash_it != hash_.end())
      list_.moveToBack(hash_it->second.getNode());
  }


  iterator find(const K& id)
  {
    ObjMapIterator hash_it = hash_.find(id);
    if (hash_it != hash_.end())
    {
      return CacheIterator<K, T>(this, hash_it->second);
    }
    return CacheIterator<K, T>();
  }

  void erase(iterator &it)
  {
    hash_.erase(it.id());
    list_.erase(it.getNode());
  }

#if 0
  bool deprecated_find(const K& id, T* & object, bool move_to_front=false)
  {
    // try to find node in buffer
    ObjMapIterator hash_it = hash_.find(id);
    if (hash_it != hash_.end())
    {
      if (move_to_front)
        list_.moveToFront(hash_it->second.getNode());        

      object = hash_it->second.get().object;

      return true;
    }
    return false;
  }
#endif


  void clear()
  {
    list_.clear();
    hash_.clear();
  }


  void push_back(const K& id, T* object)
  {
    list_.push_back(Storage(id, object));
    hash_.insert(typename std::pair<K, ObjListIterator>(id, list_.backIterator()));
  }


  void push_front(const K& id, T* object)
  {
    list_.push_front(Storage(id, object));
    hash_.insert(typename std::pair<K, ObjListIterator>(id, list_.frontIterator()));
  }

  void pop_back()
  {
    hash_.erase(list_.back().id);
    list_.pop_back();
  }

  size_t size() const
  {
    return hash_.size();
  }

  T* front()
  {
    return list_.front().object;
  }

  T* back()
  {
    return list_.back().object;
  }


  ObjList list_;
  ObjMap hash_;


private:
};




template <class K, class T>
class CacheIterator
{
public:
  // TODO: CacheIterator should be moved inside Cache
  // TODO: CacheIterator should not be constructed outside of Cache, only being given out by calls on Cache itself.
  CacheIterator() : b(NULL) {}
  CacheIterator(Cache<K, T>* _b): 
    b(_b),
    it(b.list_.frontIterator()){};

  CacheIterator(Cache<K, T>* _b, typename Cache<K, T>::ObjListIterator _it): 
    b(_b),
    it(_it){};

  CacheIterator(const CacheIterator<K, T> & ci): 
    b(ci.b),
    it(ci.it){};

  void operator =(const CacheIterator<K, T> & ci)
  {
    b = ci.b;
    it = ci.it;
  }

  void next()
  {
    it.next();
  }
  void previous()
  {
    it.previous();
  }

  T* get()
  {
    return it.get().object;
  }

  ListNode<typename Cache<K, T>::Storage>* getNode()
  {
    return it.getNode();
  }

  K id()
  {
    return it.get().id;
  }

  bool finished()
  {
    return b == NULL || it.finished();
  }

  bool operator==(const CacheIterator<K, T> &o) const
  {
    return b == o.b && it == o.it;
  }
  
private:
  Cache<K, T>* b;
  typename Cache<K, T>::ObjListIterator it;
};


}

#endif
