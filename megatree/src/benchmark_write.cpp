#include <cstdio>
#include <cstdlib>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <megatree/common.h>
#include <megatree/storage_factory.h>
#include <megatree/megatree.h>
#include <megatree/tree_functions.h>

using namespace megatree;


const std::string TREE_PATH = "benchmark_tree";
const double SCANNER_RANGE = 30;
const int POINTS_PER_SCAN = 1000;
const double CAR_STEP = 0.5;
const double SCAN_ACCURACY = 0.001;  // 1 mm accuracy of scans


int main (int argc, char** argv)
{
  if (argc != 5 && argc != 6)
  {
    printf("Usage: ./benchmark_write   subtree_width  subfolder_depth  cache_size  num_scans [tree]\n");
    return -1;
  }
  int NUM_SCANS = parseNumberSuffixed(argv[4]);

  std::string tree_path = TREE_PATH;
  if (argc > 5)
    tree_path = argv[5];

  printf("sizeof(Node) = %zu\n", sizeof(Node));

  boost::posix_time::ptime started, finished, flushed;

  // Deletes any existing tree
  megatree::removePath(tree_path);
  
  double scan_accuracy = SCAN_ACCURACY;
  if (argc > 6){
    scan_accuracy = atoi(argv[6]);
  }
    

  // create megatree
  std::vector<double> tree_center(3, 0);
  double tree_size = 6378000+8850; // radius of the earth + height of Mount Everest
  boost::shared_ptr<Storage> storage(openStorage(tree_path));
  MegaTree tree(storage, tree_center, tree_size, atoi(argv[1]), atoi(argv[2]), 
                parseNumberSuffixed(argv[3]), scan_accuracy);

  // Generates lots of random numbers
  srand(32423);
  unsigned int num_random_numbers = 3 * POINTS_PER_SCAN * 100;
  std::vector<double> random_numbers;
  
  random_numbers.reserve(num_random_numbers);
  for (size_t i = 0; i < num_random_numbers; ++i) 
  {
    double rand = drand48() * SCANNER_RANGE;
    rand = trunc(rand*1000)/1000;
    random_numbers.push_back(rand);
  }
  std::vector<double> pt(3, 0.0);
  std::vector<double> white(3, 255);

  // adding points to tree
  printf("Adding %d sorted points.\n", POINTS_PER_SCAN);    
  started = boost::posix_time::microsec_clock::universal_time();

  for (int i = 0; i < NUM_SCANS; ++i)
  {
    if (i*POINTS_PER_SCAN %100000 == 0)
    {
      printf("Adding point %.1fM, time passed %.1f min, %s\n", 
             i*POINTS_PER_SCAN/1e6, 
             (boost::posix_time::microsec_clock::universal_time()-started).total_seconds()/60., 
             tree.toString().c_str());
      dumpTimers();
      tree.resetCount();
    }
    
    for (int j=0; j<POINTS_PER_SCAN; j++)
    {
      pt[0] = ((double)i*CAR_STEP);
      pt[1] = random_numbers[(3*j+1)%num_random_numbers];
      pt[2] = random_numbers[(3*j+2)%num_random_numbers];
      addPoint(tree, pt, white);
    }
  }
  dumpTimers();
  finished = boost::posix_time::microsec_clock::universal_time();
  tree.flushCache();
  flushed = boost::posix_time::microsec_clock::universal_time();
  printf("Adding %d points took  %.3f seconds. Writing to disk took %.3f seconds\n",
         NUM_SCANS * POINTS_PER_SCAN,
         (flushed - started).total_milliseconds() / 1000.0f,
         (flushed - finished).total_milliseconds() / 1000.0f);
  
  return 0;
}
