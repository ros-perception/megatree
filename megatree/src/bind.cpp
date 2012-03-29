#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <cstdio>
#include <megatree/function_caller.h>


using namespace megatree;

void function(int i)
{
  printf("Calling function with i=%d\n", i);
  sleep(.1);
}


int main()
{
  FunctionCaller f(10);
  
  for (unsigned i=0; i<99; i++)
    f.addFunction(boost::bind(&function, i));

  sleep(3);
  return 0;

}
