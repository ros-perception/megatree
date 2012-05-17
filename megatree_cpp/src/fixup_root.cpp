#include <argp.h>
#include <stdio.h>
#include <string.h>

#include <megatree/node_file.h>
#include <megatree/storage.h>
#include <megatree/storage_factory.h>

using namespace megatree;

struct arguments_t {
  char* tree;
};


static int parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments_t *arguments = (arguments_t*)state->input;
  
  switch (key)
  {
  case 't':
    arguments->tree = arg;
    break;
  case ARGP_KEY_ARG:
    break;
  }
  return 0;
}


int main (int argc, char** argv)
{
  // Default command line arguments
  struct arguments_t arguments;
  arguments.tree = 0;
  
  // Parses command line options
  struct argp_option options[] = {
    {"tree",       't', "TREE",   0,     "Path to tree"},
    { 0 }
  };
  struct argp argp = { options, parse_opt };
  int parse_ok = argp_parse(&argp, argc, argv, 0, 0, &arguments);
  printf("Arguments parsed: %d\n", parse_ok);
  printf("Tree: %s\n", arguments.tree);
  assert(parse_ok == 0);

  if (!arguments.tree) {
    fprintf(stderr, "No tree path given.\n");
    return 1;
  }

  boost::shared_ptr<Storage> storage(openStorage(arguments.tree));

  // Retrieves file f1
  ByteVec f1_binary;
  storage->get("f1", f1_binary);
  if (f1_binary.empty())
  {
    fprintf(stderr, "The f1 file was not found for tree: %s\n", arguments.tree);
    exit(1);
  }

  NodeFile f1("f1");
  f1.deserialize(f1_binary);
  printf("Loaded f1 with %u nodes\n", f1.cacheSize());

  NodeFile f("f");
  f.initializeRootNodeFile(boost::filesystem::path("f"), f1);
  ByteVec f_binary;
  f.serialize(f_binary);
  storage->put("f", f_binary);
  f.setWritten();

  // Writes the metadata.  This is currently copied out of megatree,
  // which is less than ideal.  Also, these parameters are duplicated
  // throughout the process, which is also less than ideal.
  std::stringstream output;
  output << "version = " << 11 << std::endl;  // Must match how the mapreduces work.  Change this with great caution.
  output << "min_cell_size = " << 0.001 << std::endl;
  output << "subtree_width = " << 6 << std::endl;
  output << "subfolder_depth = " << 10000000 << std::endl;
  output << "tree_center_x = " << 0 << std::endl;
  output << "tree_center_y = " << 0 << std::endl;
  output << "tree_center_z = " << 0 << std::endl;
  output << "tree_size = " << (2 * (6378000.0 + 8850.0)) << std::endl;

  ByteVec metadata_bytes(output.str().size());
  memcpy(&metadata_bytes[0], &output.str()[0], output.str().size());
  storage->put("metadata.ini", metadata_bytes);
  
  /*
  FILE* file = fopen("f_binary", "wb");
  assert(file);
  fwrite(&f_str[0], 1, f_str.size(), file);
  fclose(file);
  */
  
  printf("Finished.\n");
  return 0;
}
