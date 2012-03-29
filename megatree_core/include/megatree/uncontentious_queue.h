#ifndef MEGATREE_UNCONTENTIOUS_QUEUE_H
#define MEGATREE_UNCONTENTIOUS_QUEUE_H

#include <boost/thread.hpp>

namespace megatree {

// http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html#tlq
//
// head always points to a dead node.  It's either the empty node from
// initialization, or it's a node whose value has already been
// returned.
template <class T>
class UncontentiousQueue
{
public:
  UncontentiousQueue();
  ~UncontentiousQueue();

  void enqueue(const T&);
  bool dequeue(T&);
  void dequeueBlocking(T&);

private:
  struct Node
  {
    T data;
    Node* next;
  };
  
  boost::mutex head_mutex, tail_mutex;
  boost::condition grown_condition;
  Node* head;
  Node* tail;
};

template <class T>
UncontentiousQueue<T>::UncontentiousQueue()
{
  Node* dummy = new Node;
  dummy->next = NULL;
  head = tail = dummy;
}

template <class T>
UncontentiousQueue<T>::~UncontentiousQueue()
{
  boost::mutex::scoped_lock head_lock(head_mutex), tail_lock(tail_mutex);
  Node *n = head;
  while (n) {
    Node* old = n;
    n = n->next;
    delete old;
  }
}

template <class T>
void UncontentiousQueue<T>::enqueue(const T& item)
{
  Node* n = new Node;
  n->data = item;
  n->next = NULL;

  boost::mutex::scoped_lock lock(tail_mutex);
  tail->next = n;
  tail = n;
  grown_condition.notify_one();
}

template <class T>
bool UncontentiousQueue<T>::dequeue(T& item)
{
  boost::mutex::scoped_lock lock(head_mutex);
  Node* new_head = head->next;
  if (new_head == NULL)
    return false;
  item = new_head->data;
  delete head;
  head = new_head;
  return true;
}

template <class T>
void UncontentiousQueue<T>::dequeueBlocking(T& item)
{
  boost::mutex::scoped_lock lock(head_mutex);
  while (head->next == NULL)
    grown_condition.wait(head_mutex);
  
  Node* new_head = head->next;
  assert(new_head != NULL);
  item = new_head->data;
  delete head;
  head = new_head;
  return true;
}

} // namespace

#endif
