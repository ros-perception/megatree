#include <megatree/compress.h>
#include <cstdio>
#include <vector>

using namespace megatree;

class A
{
public:
  A(){}

  A(const A& a)
  {
    printf("copy constructor\n");
  }
};

int main()
{
  std::vector<A> v;
  v.reserve(3);
  A a;
  v.push_back(a);
  v.push_back(a);
  v.push_back(a);  

  v.resize(2);
  return 1;

  uint16_t num = -1;

  for (uint16_t pnt=0; pnt<num; pnt++)
  {
    uint64_t tmp;
    bitcpy(tmp, pnt, pnt%10, 13, 3);
    printf("Point %d, tmp %d\n", (int)pnt, (int)tmp);
  }


}
