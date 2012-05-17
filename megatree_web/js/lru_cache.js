
function List()
{
  this.front = null;
  this.back = null;
}
List.prototype.push_front = function(item)
{
  var iter = new Object();
  iter.item = item;
  iter.prev = null;
  iter.next = this.front;
  if (this.front != null) {
    this.front.prev = iter;
  }
  this.front = iter;

  if (this.back == null)
    this.back = iter;

  return iter;
}
List.prototype.push_back = function(item)
{
  var iter = new Object();
  iter.item = item;
  iter.prev = this.back;
  iter.next = null;
  if (this.back != null) {
    this.back.next = iter;
  }
  this.back = iter;

  if (this.front == null)
    this.front = iter;

  return iter;
}

List.prototype.pop_front = function()
{
  if (this.front == null)
    return null;

  //remove the front node from the list
  var new_front = this.front.next;

  if (new_front != null)
    new_front.prev = null;

  //if the front and the back of the list are the same, then we need to set
  //the back to NULL
  if (this.front == this.back)
    this.back = null;

  var old_front = this.front;
  this.front = new_front;
  return old_front.item;
}

List.prototype.pop_back = function()
{
  if (this.back == null)
    return null;

  // remove the back node from the list
  var new_back = this.back.prev;

  // if the front and the back of the list are the same, then we need to set
  // the back to NULL
  if (this.front == this.back)
      this.front = null;

  if (new_back != null)
      new_back.next = null;

  var old_back = this.back;
  this.back = new_back;
  return old_back.item;
}


List.prototype.remove = function(iter)
{
 if (iter == this.front && iter == this.back)
   {
     this.front = this.back = null;
   }
   else if (iter == this.front)
   {
     this.front = iter.next;
     iter.next.prev = null;
   }
   else if (iter == this.back)
   {
     this.back = iter.prev;
     iter.prev.next = null;
   }
   else
   {
     iter.prev.next = iter.next;
     iter.next.prev = iter.prev;
   }

  iter.next = iter.prev = null;
  return iter;

}

List.prototype.move_to_front = function(iter)
{
  //if the node is already on the front, we'll just return
  if (iter == this.front){
    return;
  }

  // first, we want to remove the node from its current location in the list
  iter.prev.next = iter.next;

  // if the node is the back, we need to change that pointer
  if (iter == this.back){
    this.back = iter.prev;
  }
  else{
    iter.next.prev = iter.prev;
  }

  // next, we want to put the node on the front of the list
  iter.prev = null;
  iter.next = this.front;

  this.front.prev = iter;
  this.front = iter;
}

List.prototype.clear = function()
{
  this.front = null;
  this.back = null;
}


function LRUCache(max_size)
{
  this.max_size = max_size;
  this.size = 0;
  this.lookup = {};
  this.list = new List();
}

LRUCache.prototype._enforceOverflow = function()
{
  while (this.size > this.max_size) {
    var removed = this.list.pop_back();
    delete this.lookup[removed.id];
    --this.size;
  }
}

LRUCache.prototype.put = function(id, item)
{
  var existing = this.lookup[id];
  if (existing) {
    existing.item.item = item;
    this.list.move_to_front(existing);
  }
  else {
    var node = {"id": id, "item": item};
    var iter = this.list.push_front(node);
    ++this.size;
    this.lookup[id] = iter;
    this._enforceOverflow();
  }
}

LRUCache.prototype.get = function(id)
{
  var existing = this.lookup[id];
  if (existing) {
    this.list.move_to_front(existing);
    return existing.item.item;
  }
  return null;
}

LRUCache.prototype.clear = function()
{
  this.size = 0;
  this.lookup = {};
  this.list.clear();
}

