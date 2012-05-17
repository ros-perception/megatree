#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <list>
#include <vector>
#include <cstdio>
#include <megatree/long_id.h>
#include <megatree/tree_functions.h>


using namespace megatree;


const unsigned size = 100000000;
int main()
{
  boost::posix_time::ptime started, finished;

  /*
  std::list<int> l;
  std::vector<int> res;
  res.reserve(size);
  for (unsigned i=0; i<size; i++)
    l.push_back(3);


  started = boost::posix_time::microsec_clock::universal_time();
  for (std::list<int>::iterator it=l.begin(); it != l.end(); it++)
  {
    res.push_back(*it);
  }
  
  finished = boost::posix_time::microsec_clock::universal_time();
  printf("Looped through list of size %d in %.3f seconds\n", size,
         (finished - started).total_milliseconds() / 1000.0f);


  IdType id(0153422736352435267);
  started = boost::posix_time::microsec_clock::universal_time();
  for (unsigned i=0; i<size; i++)
  {
    IdType tmp = getFileId(id, 1, 6);
  }  
  finished = boost::posix_time::microsec_clock::universal_time();
  printf("Get file id %d times took  %.3f seconds\n", size,
         (finished - started).total_milliseconds() / 1000.0f);

  */
}
