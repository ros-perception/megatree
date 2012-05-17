#include <megatree/node.h>
#include <megatree/long_id.h>
#include <megatree/cache.h>
#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>
#include <tr1/unordered_map>

const int SIZE=10000000;
//const int SIZE=1000000;

using namespace megatree;

int main()
{
  printf("sizeof(Node) = %zu\n", sizeof(Node));

  std::vector<Node> v;
  v.resize(SIZE);

  /*
  Cache<IdType, Node> c;
  for (size_t i = 0; i < SIZE; ++i) {
    c.push_back(i, boost::shared_ptr<Node>(new Node));
  }
  */
  

  while(true)
  {
    sleep(1);
  }

  return 0;
}
