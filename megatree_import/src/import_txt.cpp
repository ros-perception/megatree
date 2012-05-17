#include <unistd.h>
#include <iostream>
#include <argp.h>

#include <boost/algorithm/string.hpp>

#include <megatree/common.h>
#include <megatree/megatree.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>

#include <fstream>
#include <iostream>

using namespace megatree;

static unsigned long total_points = 0;
static Tictoc overall_timer;

void importTxt(MegaTree &tree, const boost::filesystem::path &path, unsigned long max_intensity, unsigned int *skip = NULL)
{
  printf("Loading txt file: %s\n", path.string().c_str());
  std::ifstream fin;
  fin.open(path.string().c_str(), std::ios::in);

  char line[1024];
  fin.getline(line, 1024);  // Skips the first line

  // Adds each point into the tree
  unsigned i = 0;
  std::vector<double> point(3, 0.0);
  std::vector<double> color(3, 0.0);

  while (!fin.eof())
  {
    fin.getline(line, 1024);

    std::vector<std::string> bits;
    boost::split(bits, line, boost::is_any_of(" ,"));
    if (bits.size() != 6)
      continue;

    point[0] = atof(bits[0].c_str());
    point[1] = atof(bits[1].c_str());
    point[2] = atof(bits[2].c_str());

    color[0] = atoi(bits[3].c_str());
    color[1] = atoi(bits[4].c_str());
    color[2] = atoi(bits[5].c_str());
    
    if (i % 100000 == 0) {
      //printf("Adding %u\n", i);
      printf("%8.1f   %s\n", overall_timer.toc(), tree.toString().c_str());
      tree.resetCount();
    }
    addPoint(tree, point, color);

    ++total_points;
    /*
    if (total_points % 10000000L == 0)
    {
      printf("Flushing %lu points\n", total_points);
      tree.flushCache();
    }
    */
    
    ++i;
  }
  printf("%s\n", tree.toString().c_str());
  tree.resetCount();
}


struct arguments_t {
  int cache_size;
  char* tree;
  unsigned int skip;
  unsigned long max_intensity;
  std::vector<std::string> las_filenames;
};


static int parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments_t *arguments = (arguments_t*)state->input;
  
  switch (key)
  {
  case 'c':
    //arguments->cache_size = atoi(arg);
    arguments->cache_size = parseNumberSuffixed(arg);
    break;
  case 't':
    arguments->tree = arg;
    break;
  case 's':
    arguments->skip = (unsigned int)parseNumberSuffixed(arg);
    break;
  case 'i':
    arguments->max_intensity = atol(arg);
    break;
  case ARGP_KEY_ARG:
    arguments->las_filenames.push_back(arg);
    break;
  }
  return 0;
}


int main (int argc, char** argv)
{
  // Default command line arguments
  struct arguments_t arguments;
  arguments.cache_size = 3000000;
  arguments.max_intensity = 255;  // 16384
  arguments.skip = 0;
  arguments.tree = 0;
  
  // Parses command line options
  struct argp_option options[] = {
    {"cache-size", 'c', "SIZE",   0,     "Cache size"},
    {"tree",       't', "TREE",   0,     "Path to tree"},
    {"skip",       's', "SKIP",   0,     "Number of points to skip"},
    {"max-intensity",  'i', "INTENSITY",  0,     "Maximum intensity value"},
    { 0 }
  };
  struct argp argp = { options, parse_opt };
  int parse_ok = argp_parse(&argp, argc, argv, 0, 0, &arguments);
  printf("Arguments parsed: %d\n", parse_ok);
  printf("Cache size: %d\n", arguments.cache_size);
  printf("Tree: %s\n", arguments.tree);
  if (arguments.skip > 0)
    printf("Skipping %u points\n", arguments.skip);
  assert(parse_ok == 0);

  if (!arguments.tree) {
    fprintf(stderr, "No tree path given.\n");
    return 1;
  }

  boost::shared_ptr<Storage> storage(openStorage(arguments.tree));
  MegaTree tree(storage, arguments.cache_size, false);

  overall_timer.tic();
  
  Tictoc timer;
  for (size_t i = 0; i < arguments.las_filenames.size(); ++i)
  {
    boost::filesystem::path las_path(arguments.las_filenames[i]);
    printf("Importing %s into tree\n", las_path.string().c_str());

    Tictoc one_file_timer;
    importTxt(tree, las_path, arguments.max_intensity, &arguments.skip);
    float t = one_file_timer.toc();
    printf("Flushing %s...\n", las_path.string().c_str());
    tree.flushCache();
    printf("Finished %s in %.3lf seconds (%.1lf min or %.1lf hours)\n",
           las_path.string().c_str(), t, t/60.0f, t/3600.0f);
  }

  printf("Flushing the cache\n");
  tree.flushCache();
  
  float t = timer.toc();
  unsigned long num_points = tree.getNumPoints();
  printf("Importing all files took %.3f seconds for %lu points ==> %.3f sec/million\n", t, num_points, t / num_points * 1e6);

  printf("Finished.\n");
  return 0;
}
