#include <megatree/common.h>
#include <megatree/megatree.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>
#include <opencv2/opencv.hpp>

#include <unistd.h>
#include <iostream>
#include <argp.h>

#include <fstream>
#include <iostream>

using namespace megatree;

static const double radius = 6378000/2.0;


void importImage(MegaTree &tree, const boost::filesystem::path &path)
{
  printf("Loading image file: %s\n", path.string().c_str());
  cv::Mat_<cv::Vec3b> image = cv::imread(path.string(), 1);
  std::cout << "Points count: " << image.rows * image.cols << std::endl;;

  // Adds each point into the tree
  std::vector<double> point(3, 0.0);
  std::vector<double> color(3, 0.0);
  unsigned count = 0;
  for (int i=0; i<image.cols; i++)
  {
    for (int j=0; j<image.rows; j++)
    {
      double p = (double)i / (double)image.cols * (2.0 * M_PI);
      double q = (double)j / (double)image.rows * M_PI - M_PI / 2.0;

      point[0] = radius * cos(p) * cos(q);
      point[1] = radius * sin(q);
      point[2] = radius * sin(p) * cos(q);
      color[0] = image(j, i)[2];
      color[1] = image(j, i)[1];
      color[2] = image(j, i)[0];

      addPoint(tree, point, color);
      count++;
      if (count % 100000 == 0)
        printf("%s\n", tree.toString().c_str());        

    }
  }
  printf("%s\n", tree.toString().c_str());
}


struct arguments_t {
  int cache_size;
  char* tree;
  std::vector<std::string> filenames;
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
  case ARGP_KEY_ARG:
    arguments->filenames.push_back(arg);
    break;
  }
  return 0;
}


int main (int argc, char** argv)
{
  // Default command line arguments
  struct arguments_t arguments;
  arguments.cache_size = 3000000;
  arguments.tree = 0;
  
  // Parses command line options
  struct argp_option options[] = {
    {"cache-size", 'c', "SIZE",   0,     "Cache size"},
    {"tree",       't', "TREE",   0,     "Path to tree"},
    { 0 }
  };
  struct argp argp = { options, parse_opt };
  int parse_ok = argp_parse(&argp, argc, argv, 0, 0, &arguments);
  printf("Arguments parsed: %d\n", parse_ok);
  printf("Cache size: %d\n", arguments.cache_size);
  printf("Tree: %s\n", arguments.tree);
  assert(parse_ok == 0);

  if (!arguments.tree) {
    fprintf(stderr, "No tree path given.\n");
    return 1;
  }

  boost::shared_ptr<Storage> storage(openStorage(arguments.tree));
  MegaTree tree(storage, arguments.cache_size, false);

  Tictoc timer;
  for (size_t i = 0; i < arguments.filenames.size(); ++i)
  {
    boost::filesystem::path path(arguments.filenames[i]);
    printf("Importing %s into tree\n", path.string().c_str());

    Tictoc one_file_timer;
    importImage(tree, path);
    float t = one_file_timer.toc();
    printf("Finished %s in %.3lf seconds (%.1lf min or %.1lf hours)\n",
           path.string().c_str(), t, t/60.0f, t/3600.0f);
  }

  printf("Flushing the cache\n");
  tree.flushCache();
  
  float t = timer.toc();
  unsigned long num_points = tree.getNumPoints();
  printf("Importing all files took %.3f seconds for %lu points ==> %.3f sec/million\n", t, num_points, t / num_points * 1e6);

  printf("Finished.\n");
  return 0;
}
