#include <argp.h>
#include <unistd.h>
#include <iostream>
#include <vector>

#include <megatree/common.h>
#include <megatree/megatree.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>

using namespace megatree;


struct Analysis
{
  Analysis()
    : num_nodes(0), num_leaves(0), max_depth(0)
  {
    memset(child_count_distr, 0, 9*sizeof(unsigned long));
    count_distr.resize(22);
    tendril_length_distr.resize(12);
  }

  unsigned long num_nodes;
  unsigned long num_leaves;
  unsigned int max_depth;
  unsigned long child_count_distr[9];  // Number of nodes with n children
  std::vector<unsigned long> leaf_depth_distr;  // Number of leaves with depth n
  std::vector<unsigned long> count_distr;
  std::vector<unsigned long> tendril_length_distr;

  // State variables for the current run.
  unsigned long tendril_length;

  void dump()
  {
    printf("analysis:\n");
    printf("  num_nodes:    %16lu\n", num_nodes);
    printf("  num_leaves:   %16lu\n", num_leaves);
    printf("  max_depth:    %16u\n", max_depth);

    printf("  child_count_distr: [");
    for (size_t i = 0; i < 8; ++i)
      printf(" %10lu,", child_count_distr[i]);
    printf(" %10lu ]\n", child_count_distr[8]);

    printf("  count_distr: [");
    for (size_t i = 0; i < count_distr.size() - 1; ++i)
      printf(" %6lu,", count_distr[i]);
    printf(" %6lu ]\n", count_distr.back());

    printf("  tendril_length_distr: [");
    for (size_t i = 0; i < tendril_length_distr.size() - 1; ++i)
      printf(" %6lu,", tendril_length_distr[i]);
    printf(" %6lu ]\n", tendril_length_distr.back());

    printf("\n\n");
    printf("Tendril nodes:   %10lu\n", count_distr[1] - num_leaves);
    printf("Stringy nodes:   %10lu\n", child_count_distr[1]);
    printf("\n\n");
    
  }
};

void analyze(MegaTree &tree, NodeHandle& node, Analysis& a, unsigned int depth=0)
{
  if (a.num_nodes % 100000L == 0) {
    printf("At %5.1lfM: %s\n", double(a.num_nodes) / double(1000000), tree.toString().c_str());
    tree.resetCount();
  }
  a.num_nodes++;
  if (node.isLeaf())
    a.num_leaves++;
  a.max_depth = std::max(a.max_depth, depth);
  
  // Updates child count distribution
  int num_children = 0;
  for (size_t i = 0; i < 8; ++i) {
    if (node.hasChild(i))
      ++num_children;
  }
  a.child_count_distr[num_children]++;

  // Updates the leaf depth distribution.
  if (node.isLeaf()) {
    if (a.leaf_depth_distr.size() < depth + 1)
      a.leaf_depth_distr.resize(depth + 1);
    a.leaf_depth_distr[depth]++;
  }

  // Updates the node count distribution.
  if (node.getCount() >= a.count_distr.size() - 1)
    ++a.count_distr.back();
  else
    ++a.count_distr[node.getCount()];

  // Updates tendril statistics.
  if (node.isLeaf() && a.tendril_length > 0)
  {
    if (a.tendril_length >= a.tendril_length_distr.size())
      ++a.tendril_length_distr.back();
    else
      ++a.tendril_length_distr[a.tendril_length];
  }
  else if (node.getCount() == 1)
  {
    printf("node with count 1 that is not leaf\n");
    ++a.tendril_length;
  }
  else
    a.tendril_length = 0;

  // Descend
  for (MegaTree::ChildIterator it(tree, node); !it.finished(); it.next())
  {
    analyze(tree, it.getChildNode(), a, depth + 1);
  }
}

struct arguments_t {
  int cache_size;
  char* tree;
};

static int parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments_t *arguments = (arguments_t*)state->input;
  
  switch (key)
  {
  case 'c':
    arguments->cache_size = parseNumberSuffixed(arg);
    break;
  case 't':
    arguments->tree = arg;
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

  if (!arguments.tree) {
    fprintf(stderr, "No tree path given.\n");
    return 1;
  }


  boost::shared_ptr<Storage> storage(openStorage(arguments.tree));
  MegaTree tree(storage, arguments.cache_size, true);
  Analysis analysis;

  Tictoc overall_timer;
  NodeHandle root;
  tree.getRoot(root);
  analyze(tree, root, analysis);
  tree.releaseNode(root);
  float overall_time = overall_timer.toc();

  analysis.dump();
  printf("Walked %'lu points in %.3f seconds (%.1f min or %.1f hours)\n",
         tree.getNumPoints(), overall_time, overall_time/60.0f, overall_time/3600.0f);

  
  return 0;
}
