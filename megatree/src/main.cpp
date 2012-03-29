#include <pcl/io/pcd_io.h>
#include <pcl/pcl_base.h>
#include <pcl/point_types.h>
#include <pcl/filters/passthrough.h>
#include <pcl/registration/transforms.h>

#include <megatree/common.h>
#include <megatree/megatree.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>

#include <unistd.h>
#include <iostream>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <sensor_msgs/point_cloud_conversion.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>


using namespace megatree;


static double POINT_ACCURACY = 0.001;  // /1 mm accuracy of points

class TempDirectory
{
public:
  TempDirectory(const std::string &prefix = "", bool remove = true)
    : remove_(remove)
  {
    std::string tmp_storage = prefix + "XXXXXX";
    char *tmp = mkdtemp(&tmp_storage[0]);
    assert(tmp);
    printf("Temporary directory: %s\n", tmp);

    path_ = tmp;
  }

  ~TempDirectory()
  {
    if (remove_)
      boost::filesystem::remove_all(path_);
  }

  const boost::filesystem::path &getPath() const
  {
    return path_;
  }

private:
  boost::filesystem::path path_;
  bool remove_;
};


void importPointCloudBag(MegaTree &tree, const boost::filesystem::path & path, bool flip_r_and_b)
{
  rosbag::Bag bag;
  
  bag.open(path.string(), rosbag::bagmode::Read);

  std::vector<std::string> topics;
  topics.push_back("clouds");

  rosbag::View view(bag, rosbag::TopicQuery(topics));
  int cloud_count = 0;
  BOOST_FOREACH(rosbag::MessageInstance const m, view)
  {
    printf("Cloud %d\n", cloud_count++);
    sensor_msgs::PointCloud::ConstPtr c = m.instantiate<sensor_msgs::PointCloud>();
    if(c != NULL)
    {
      sensor_msgs::PointCloud2 pc2;
      if(!sensor_msgs::convertPointCloudToPointCloud2(*c, pc2))
        printf("Error in conversion\n");
      else
      {
        boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> > cloud_nan(new pcl::PointCloud<pcl::PointXYZRGB>);
        boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> > cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
        pcl::fromROSMsg(pc2, *cloud_nan);

        pcl::PassThrough<pcl::PointXYZRGB> pass_filter; 
        pass_filter.setInputCloud(cloud_nan);
        pass_filter.setFilterFieldName("z");
        pass_filter.setFilterLimits(1e-5, 9999999);
        pass_filter.filter(*cloud);

        boost::posix_time::ptime started, finished, flushed;
        started = boost::posix_time::microsec_clock::universal_time();
        std::vector<double> pnt(3);
        std::vector<double> color(3);
        for (unsigned int i=0; i<cloud->size(); i++)
        {
          if (i % 100000 == 0)
            printf("Adding point %d\n", (int)i);

          //only add points with a certain z value to filter out lousy scans
          pnt[0] = cloud->points[i].x;
          pnt[1] = cloud->points[i].y;
          pnt[2] = cloud->points[i].z;

          if(!flip_r_and_b)
          {
            color[0] = cloud->points[i].r;
            color[1] = cloud->points[i].g;
            color[2] = cloud->points[i].b;
          }
          else
          {
            color[0] = cloud->points[i].b;
            color[1] = cloud->points[i].g;
            color[2] = cloud->points[i].r;
          }
          addPoint(tree, pnt, color);
        }

      }
    }
  }
}

void importPCDFile(MegaTree &tree, const boost::filesystem::path &path, double z_threshold = 0.0)
{
  boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> > cloud_nan(new pcl::PointCloud<pcl::PointXYZRGB>);
  boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> > cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
  pcl::io::loadPCDFile(path.string(), *cloud_nan);
  pcl::PassThrough<pcl::PointXYZRGB> pass_filter; 
  pass_filter.setInputCloud(cloud_nan);
  pass_filter.setFilterFieldName("z");
  pass_filter.setFilterLimits(1e-5, 9999999);
  pass_filter.filter(*cloud);

  boost::posix_time::ptime started, finished, flushed;
  started = boost::posix_time::microsec_clock::universal_time();
  std::vector<double> pnt(3);
  std::vector<double> color(3);
  for (unsigned int i=0; i<cloud->size(); i++)
  {
    if (i % 100000 == 0)
      printf("Adding point %d\n", (int)i);

    //only add points with a certain z value to filter out lousy scans
    if(z_threshold == 0.0 || cloud->points[i].z < z_threshold)
    {
      pnt[0] = cloud->points[i].x;
      pnt[1] = cloud->points[i].y;
      pnt[2] = cloud->points[i].z;
      color[0] = cloud->points[i].r;
      color[1] = cloud->points[i].g;
      color[2] = cloud->points[i].b;
      addPoint(tree, pnt, color);
    }
  }
  finished = boost::posix_time::microsec_clock::universal_time();
  tree.flushCache();
  flushed = boost::posix_time::microsec_clock::universal_time();
  printf("Adding %zu points took  %.3f seconds. Writing to disk took %.3f seconds\n", cloud->size(),
         (flushed - started).total_milliseconds() / 1000.0f,
         (flushed - finished).total_milliseconds() / 1000.0f);
}

void importScans(MegaTree &tree, const boost::filesystem::path &path, double z_threshold = 0.0)
{
  // Finds the PCD files
  std::vector<std::string> pcd_files;
  std::vector<boost::filesystem::path> pcd_paths;
  for (boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
  {
    if (it->path().extension() == boost::filesystem::path(".pcd"))
    {
      pcd_paths.push_back(it->path());
      pcd_files.push_back(it->path().string());
    }
  }

  // Sorts the PCD filenames
  std::sort(pcd_paths.begin(), pcd_paths.end());

  if (pcd_paths.empty()) {
    fprintf(stderr, "No PCD files were found in %s\n", path.string().c_str());
    return;
  }

  // Checks if pose files exist in general.
  bool pose_files_exist = false;
  boost::filesystem::path a_pose_path = pcd_paths[0];
  a_pose_path.replace_extension(std::string(".pose"));
  if (boost::filesystem::exists(a_pose_path)) {
    printf("Pose files exist\n");
    pose_files_exist = true;
  }
  else {
    printf("No pose files found\n");
    pose_files_exist = false;
  }
  
  // Timing
  boost::posix_time::ptime started, finished, flushed;
  started = boost::posix_time::microsec_clock::universal_time();

  // Loops through the directory, importing each pcd file
  unsigned long total_points = 0;
  for (size_t i = 0; i < pcd_paths.size(); ++i)
  {
    printf("File: %s\n", pcd_paths[i].string().c_str());

    // Finds the corresponding pose file
    boost::filesystem::path pose_path = pcd_paths[i];
    pose_path.replace_extension(".pose");
    if (boost::filesystem::exists(pose_path) != pose_files_exist) {
      fprintf(stderr, "Sometimes pose files exist, and sometimes they don't!\n");
    }

    // Loads the point cloud from disk
    boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> > cloud_nan(new pcl::PointCloud<pcl::PointXYZRGB>);
    boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> > cloud_in_camera(new pcl::PointCloud<pcl::PointXYZRGB>);
    boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> > cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::io::loadPCDFile(pcd_paths[i].string(), *cloud_nan);
    pcl::PassThrough<pcl::PointXYZRGB> pass_filter; 
    pass_filter.setInputCloud(cloud_nan);
    pass_filter.setFilterFieldName("z");
    pass_filter.setFilterLimits(1e-5, 9999999);
    pass_filter.filter(*cloud_in_camera);

    // Loads the transform.
    Eigen::Affine3f transform = Eigen::Affine3f::Identity();
    if (boost::filesystem::exists(pose_path))
    {
      std::ifstream fin(pose_path.string().c_str(), std::ifstream::in);
      double x, y, z, qx, qy, qz, qw;
      fin >> x >> y >> z >> qx >> qy >> qz >> qw;
      transform = Eigen::Translation3f(x, y, z) * Eigen::Quaternionf(qw, qx, qy, qz);
      std::cout << "Loaded transform " << transform.matrix() << std::endl;
    }

    // Transforms the point cloud.
    pcl::transformPointCloud(*cloud_in_camera, *cloud, transform);
    
    total_points += cloud->size();
    std::vector<double> pnt(3);
    std::vector<double> color(3);
    for (unsigned int j = 0; j < cloud->size(); j++)
    {
      if (j % 100000 == 0)
        printf("Cloud %zu: Adding point %u\n", i, j);

      if(z_threshold == 0.0 || cloud_in_camera->points[j].z < z_threshold)
      {
        pnt[0] = cloud->points[j].x;
        pnt[1] = cloud->points[j].y;
        pnt[2] = cloud->points[j].z;
        color[0] = cloud->points[j].r;
        color[1] = cloud->points[j].g;
        color[2] = cloud->points[j].b;
        addPoint(tree, pnt, color);
      }
    }
    printf("%s\n", tree.toString().c_str());
    tree.resetCount();
  }

  // Timing
  finished = boost::posix_time::microsec_clock::universal_time();
  tree.flushCache();
  flushed = boost::posix_time::microsec_clock::universal_time();
  printf("Adding %lu points from %zu files took  %.3f seconds. Writing to disk took %.3f seconds\n",
         total_points, pcd_paths.size(),
         (flushed - started).total_milliseconds() / 1000.0f,
         (flushed - finished).total_milliseconds() / 1000.0f);
}
  


int main (int argc, char** argv)
{

  // create megatree
  std::vector<double> tree_center(3, 0);
  double tree_size = 100;

  if(argc != 6 && argc != 7)
  {
    printf("Usage: ./main pcd_path subtree_width subfolder_depth cache_size z_threshold [tree_path]\n");
    return -1;
  }

  double point_accuracy = POINT_ACCURACY;
  TempDirectory tree_path_dir("pcd_file", false);
  boost::filesystem::path tree_path = tree_path_dir.getPath();

  if(argc > 6)
    tree_path = argv[6];

  boost::shared_ptr<Storage> storage(openStorage(tree_path));
  MegaTree tree(storage, tree_center, tree_size,
                atoi(argv[2]), atoi(argv[3]),  // subtree_width, subfolder_depth
                parseNumberSuffixed(argv[4]), point_accuracy);
  //printf("Root node is a %s\n", tree.isLeaf(1L) ? "leaf" : "node");

  // Imports a point cloud, either from a file or from a directory.
  if (argc >= 2)
  {
    boost::filesystem::path path(argv[1]);
    if (boost::filesystem::is_directory(path))
    {
      // Loads a set of clouds from the directory
      importScans(tree, path, atof(argv[5]));
    }
    else if(path.extension() == boost::filesystem::path(".bag"))
    {
      //load PointCloud messages from bags
      importPointCloudBag(tree, path, true);
    }
    else if (boost::filesystem::exists(path))
    {
      // Loads a pcd file
      importPCDFile(tree, path, atof(argv[5]));
    }
    else
    {
      printf("Path does not exist: %s\n", path.string().c_str());
    }
  }

  // Adds number of points to the tree
  else
  {
    printf("\n\nAdding 0, 0, 0\n"); 
    std::vector<double> pt;
    pt.push_back(0.0f);
    pt.push_back(0.0f);
    pt.push_back(0.0f);
    addPoint(tree, pt);

    pt.clear();
    printf("\n\nAdding 0, 1, 0\n"); 
    pt.push_back(0.0f);
    pt.push_back(1.0f);
    pt.push_back(0.0f);
    addPoint(tree, pt);

    /*
    if (tree.isLeaf(1L)) {
      boost::shared_ptr<Node> leaf = tree.getNode(1L);
      printf("Node: %s: 9%f %f %f)\n", leaf->getId().toString().c_str(), leaf->point[0], leaf->point[1], leaf->point[2]);
    }
    */
  }

  printf("The page size for this system is %ld bytes.\n",
      sysconf(_SC_PAGESIZE)); /* _SC_PAGE_SIZE is OK too. */
  
  return 0;
}
