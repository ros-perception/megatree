#include <megatree/common.h>
#include <megatree/megatree.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>

#include <unistd.h>
#include <iostream>
#include <argp.h>

#include <liblas/liblas.hpp>
#include <fstream>
#include <iostream>

using namespace megatree;

static unsigned long total_points = 0;
static Tictoc overall_timer;

void importLas(MegaTree &tree, const boost::filesystem::path &path, unsigned long max_intensity, unsigned int *skip = NULL)
{
  printf("Loading las file: %s\n", path.string().c_str());
  std::ifstream fin;
  fin.open(path.string().c_str(), std::ios::in | std::ios::binary);

  liblas::ReaderFactory f;
  liblas::Reader reader = f.CreateWithStream(fin);

  liblas::Header const& header = reader.GetHeader();
  std::cout << "Compressed: " << ((header.Compressed() == true) ? "true":"false") << "\n";
  std::cout << "Signature: " << header.GetFileSignature() << '\n';
  std::cout << "Points count: " << header.GetPointRecordsCount() << '\n';

  if (skip && *skip >= header.GetPointRecordsCount())
  {
    *skip -= header.GetPointRecordsCount();
    return;
  }

  // Hack
  //
  // Offsets the tree so its center is near the cloud.
  if (tree.getNumPoints() == 0)
  {
    printf("Recentering tree to (%lf, %lf, %lf)\n", header.GetMinX(), header.GetMinY(), header.GetMinZ());
    tree.recenter(header.GetMinX(), header.GetMinY(), header.GetMinZ());
  }

  // Determines if the file contains color information, or just intensity.5A
  // TODO: I haven't seen a LAS file with color yet, so I don't know what fields to look for.
  bool has_color = false;  // !!header.GetSchema().GetDimension("R");

  // Adds each point into the tree
  unsigned i = 0;
  std::vector<double> point(3, 0.0);
  std::vector<double> color(3, 0.0);
  while (reader.ReadNextPoint())
  {
    const liblas::Point& p = reader.GetPoint();
    point[0] = p.GetX();
    point[1] = p.GetY();
    point[2] = p.GetZ();
    if (has_color) {
      color[0] = p.GetColor().GetRed();
      color[1] = p.GetColor().GetGreen();
      color[2] = p.GetColor().GetBlue();
    }
    else {
      // The Kendall LAS files have intensity in [0,255]
      // The Node expects color values to be in [0,255] also
      double scaled_color = std::min((double)max_intensity, 255.0 * double(p.GetIntensity()) / max_intensity);
      color[0] = color[1] = color[2] = scaled_color;
      //printf("Color: %lf  from %d   and %ld\n", color[0], p.GetIntensity(), max_intensity);
      //printf("%d  ", p.GetIntensity());
    }

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
    importLas(tree, las_path, arguments.max_intensity, &arguments.skip);
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
