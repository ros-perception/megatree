#ifndef MEGATREE_LIST_H_
#define MEGATREE_LIST_H_

#include <boost/shared_ptr.hpp>
#include <stdexcept>

namespace megatree
{

  template <typename T>
  struct ListNode
  {
    ListNode<T>(const T& _object, ListNode<T>* _previous, ListNode<T>* _next) 
    : object(_object), previous(_previous), next(_next)
    {}

    T object;
    ListNode<T>* previous;
    ListNode<T>* next;

  };

  //forward declare the list iterator so that the list can use it
  template <typename T>
  class ListIterator;

  template <typename T>
  class List
  {
  public:
    List<T>():
    list_front(NULL),
    list_back(NULL)
    {}

    ~List<T>()
    {
      ListNode<T>* node = list_front;
      while(node)
      {
        ListNode<T>* next_node = node->next;
        delete node;
        node = next_node;
      }
    }

    ListIterator<T> frontIterator()
    {
      return ListIterator<T>(list_front);
    }

    ListIterator<T> backIterator()
    {
      return ListIterator<T>(list_back);
    }


    ListNode<T>* getFrontPointer()
    {
      return list_front;
    }

    ListNode<T>* getBackPointer()
    {
      return list_back;
    }

    void push_front(const T& object)
    {
      //create the new node
      ListNode<T>* node = new ListNode<T>(object, NULL, list_front);

      //make sure the front of the list points to it as its previous
      if(list_front)
        list_front->previous = node;

      //make sure the list stores this node as the new front
      list_front = node;

      //we also want to make this node the back if there's nothing on the back
      //of the list
      if(!list_back)
        list_back = node;
    }

    void push_back(const T& object)
    {
      //create the new node
      ListNode<T>* node = new ListNode<T>(object, list_back, NULL);

      //make sure the back of the list points to it as its next
      if(list_back)
        list_back->next = node;

      //make sure the list stores this node as the new back
      list_back = node;

      //we also want to make this node the front if there's nothing on the back
      //of the list
      if(!list_front)
        list_front = node;
    }

    T& front()
    {
      if(!list_front)
        throw std::runtime_error("You called front on an empty list!");

      return list_front->object;
    }

    void pop_front()
    {
      if(!list_front)
        return;

      //remove the front node from the list
      ListNode<T>* new_front = list_front->next;

      if(new_front)
        new_front->previous = NULL;

      //if the front and the back of the list are the same, then we need to set
      //the back to NULL
      if(list_front == list_back)
        list_back = NULL;

      delete list_front;
      list_front = new_front;
    }

    T& back()
    {
      if(!list_back)
        throw std::runtime_error("You called back on an empty list!");

      return list_back->object;
    }


    void pop_back()
    {
      if(!list_back)
        return;

      //remove the back node from the list
      ListNode<T>* new_back = list_back->previous;

      //if the front and the back of the list are the same, then we need to set
      //the back to NULL
      if(list_front == list_back)
        list_front = NULL;


      if(new_back)
        new_back->next = NULL;

      delete list_back;
      list_back = new_back;
    }

    void erase(ListNode<T>* node)
    {
      assert(node);

      if (node == list_front && node == list_back)
      {
        list_front = list_back = NULL;
      }
      else if (node == list_front)
      {
        list_front = node->next;
        node->next->previous = NULL;
      }
      else if (node == list_back)
      {
        list_back = node->previous;
        node->previous->next = NULL;
      }
      else
      {
        node->previous->next = node->next;
        node->next->previous = node->previous;
      }

      delete node;
      node = NULL;
    }

    void moveToFront(ListNode<T>* node)
    {
      assert(node);

      //if the node is already on the front, we'll just return
      if(node == list_front)
        return;

      //first, we want to remove the node from its current location in the list
      node->previous->next = node->next;

      //if the node is the back, we need to change that pointer
      if(node == list_back)
        list_back = node->previous;
      else
        node->next->previous = node->previous;

      //next, we want to put the node on the front of the list
      node->previous = NULL;
      node->next = list_front;

      list_front->previous = node;
      list_front = node;
    }

    void moveToBack(ListNode<T>* node)
    {
      assert(node);

      //if the node is already on the back, we'll just return
      if(node == list_back)
        return;

      //first, we want to remove the node from its current location in the list
      if(node->previous)
        node->previous->next = node->next;

      if(node->next)
        node->next->previous = node->previous;

      //if the node is the front, we need to change that pointer
      if(node == list_front)
        list_front = node->next;

      //next, we want to put the node on the back of the list
      node->next = NULL;
      node->previous = list_back;

      if(list_back)
        list_back->next = node;

      list_back = node;
    }

    void spliceToBack(List& splice_list)
    {
      //make sure the back of the list points to the spliced list as its next
      if(list_back && splice_list.list_front)
        list_back->next = splice_list.list_front;

      //make sure the list stores the back of the spliced list as the new back
      if(splice_list.list_back)
        list_back = splice_list.list_back;

      //we also want to make the spliced list the front if there's nothing on the back
      //of the list
      if(!list_front && splice_list.list_front)
        list_front = splice_list.list_front;

      //now, the spliced list needs to be invalidated
      splice_list.list_front = NULL;
      splice_list.list_back = NULL;
    }

    bool empty()
    {
      return list_front == NULL && list_back == NULL;
    }


  private:
    ListNode<T>* list_front;
    ListNode<T>* list_back;
  };

  template <class T>
  class ListIterator
  {
  public:
    ListIterator() : node(NULL) {}
    ListIterator(ListNode<T>* _node)
      : node(_node)
    {}

    ListIterator(const ListIterator<T> &o)
      : node(o.node)
    {
    }

    void next()
    {
      node = node->next;
    }

    void previous()
    {
      node = node->previous;
    }

    bool finished()
    {
      return !node;
    }

    ListNode<T>* getNode()
    {
      return node;
    }

    T& get()
    {
      return node->object;
    }

    bool operator==(const ListIterator<T>& o) const
    {
      return node == o.node;
    }

  private:
    ListNode<T>* node;
  };

}

#endif
