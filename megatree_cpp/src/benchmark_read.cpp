#include <cstdio>
#include <cstdlib>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <megatree/common.h>
#include <megatree/megatree.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>

const unsigned num_queries = 2;
const double resolution = 0.00001;

using namespace megatree;

int main (int argc, char** argv)
{
  if (argc < 3)
  {
    printf("Usage: ./benchmark_read  tree_path  cache_size\n");
    return -1;
  }
  boost::posix_time::ptime started, finished;

  // create megatree
  boost::shared_ptr<Storage> storage(openStorage(argv[1]));
  MegaTree tree(storage, parseNumberSuffixed(argv[2]), true);
  NodeHandle root;

  NodeGeometry root_geom(tree.getRootGeometry());
  std::vector<double> min_corner(3), max_corner(3);
  min_corner[0] = root_geom.getLo(0);
  min_corner[1] = root_geom.getLo(1);
  min_corner[2] = root_geom.getLo(2);
  max_corner[0] = root_geom.getHi(0);
  max_corner[1] = root_geom.getHi(1);
  max_corner[2] = root_geom.getHi(2);

  // query points
  std::vector<double> results, colors;
  printf("Loading points in cache with query from %f %f %f to %f %f %f\n",
         min_corner[0], min_corner[1], min_corner[2],
         max_corner[0], max_corner[1], max_corner[2]);
  started = boost::posix_time::microsec_clock::universal_time();
  queryRange(tree, min_corner, max_corner, resolution, results, colors);
  //tree.resetCount();
  printf("Received %d points from the query. %s\n", (int)results.size()/3, tree.toString().c_str());
  finished = boost::posix_time::microsec_clock::universal_time();
  tree.getRoot(root);
  printf("Loading entire tree of %d points in cache took  %.3f seconds.\n",
         (int)root.getCount(), 
         (finished - started).total_milliseconds() / 1000.0f);
  tree.releaseNode(root);

  printf("Query points\n");
  started = boost::posix_time::microsec_clock::universal_time();

  for (unsigned int i=0; i<num_queries; i++)
  {
    results.clear();
    colors.clear();
    queryRange(tree, min_corner, max_corner, resolution, results, colors);
    printf("Received %d points from the query. %s\n", (int)results.size()/3, tree.toString().c_str());
    tree.resetCount();
  }
  
  finished = boost::posix_time::microsec_clock::universal_time();

  tree.getRoot(root);
  printf("Querying entire tree of %d points %d times took  %.3f seconds.\n",
         (int)root.getCount(), num_queries,
         (finished - started).total_milliseconds() / 1000.0f);
  tree.releaseNode(root);
  return 0;
}
