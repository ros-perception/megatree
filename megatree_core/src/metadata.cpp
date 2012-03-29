#include <megatree/metadata.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <boost/program_options.hpp>


namespace megatree
{

  void MetaData::deserialize(const ByteVec& data)
  {
    // convert ByteVec to string
    std::string meta_str(data.size(), 'c');
    memcpy(&meta_str[0], &data[0], data.size());
    std::stringstream meta_stream(meta_str);

    // define options
    boost::program_options::options_description descr("MegaTree Options");
    descr.add_options()
      ("version", boost::program_options::value<unsigned>(), "Megatree version")
      ("min_cell_size", boost::program_options::value<double>(), "The minimum allowed size of a cell")
      ("subtree_width", boost::program_options::value<unsigned>(), "The width of the top of each subtree, in powers of 8")
      ("subfolder_depth", boost::program_options::value<unsigned>(), "The number of tree layers in each subfolder")
      ("tree_center_x", boost::program_options::value<double>(), "The center of the tree, x-coordinate")
      ("tree_center_y", boost::program_options::value<double>(), "The center of the tree, y-coordinate")
      ("tree_center_z", boost::program_options::value<double>(), "The center of the tree, z-coordinate")
      ("tree_size",     boost::program_options::value<double>(), "The size of the tree")
      ("default_camera_center_x",     boost::program_options::value<double>(), "The camera center, x-coordinate")
      ("default_camera_center_y",     boost::program_options::value<double>(), "The camera center, y-coordinate")
      ("default_camera_center_z",     boost::program_options::value<double>(), "The camera center, z-coordinate")
      ("default_camera_distance",     boost::program_options::value<double>(), "The camera distance")
      ("default_camera_pitch",        boost::program_options::value<double>(), "The camera pitch")
      ("default_camera_yaw",          boost::program_options::value<double>(), "The camera yaw");
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_config_file<char>(meta_stream, descr), vm);
    
    // check existance of parameters
    assert(vm.count("version"));
    assert(vm.count("min_cell_size"));
    assert(vm.count("subtree_width"));
    assert(vm.count("subfolder_depth"));
    assert(vm.count("tree_center_x"));
    assert(vm.count("tree_center_y"));
    assert(vm.count("tree_center_z"));
    assert(vm.count("tree_size"));
    
    // get params
    version = vm["version"].as<unsigned>();
    min_cell_size = vm["min_cell_size"].as<double>();
    root_size = vm["tree_size"].as<double>();
    root_center.resize(3);
    root_center[0] = vm["tree_center_x"].as<double>();
    root_center[1] = vm["tree_center_y"].as<double>();
    root_center[2] = vm["tree_center_z"].as<double>();
    subtree_width = vm["subtree_width"].as<unsigned>();
    subfolder_depth = vm["subfolder_depth"].as<unsigned>();
  }


  void MetaData::serialize(ByteVec& data)
  {
    std::stringstream output;
    output << "version = " << version << std::endl;
    output << "min_cell_size = " << min_cell_size << std::endl;
    output << "subtree_width = " << subtree_width << std::endl;
    output << "subfolder_depth = " << subfolder_depth << std::endl;
    output << "tree_center_x = " << root_center[0] << std::endl;
    output << "tree_center_y = " << root_center[1] << std::endl;
    output << "tree_center_z = " << root_center[2] << std::endl;
    output << "tree_size = " << root_size << std::endl;

    data.resize(output.str().size());
    memcpy(&data[0], &output.str()[0], data.size());
  }



} // namespace
