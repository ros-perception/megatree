#include <gtest/gtest.h>
#include <cmath>

#include <megatree/megatree.h>
#include <megatree/allocator.h>
#include <megatree/storage_factory.h>
#include <megatree/node_file.h>
#include <megatree/tree_functions.h>

using namespace megatree;

static const double point_precision = 0.001;

#define EXPECT_V3_NEAR(a, b, c, v, tolerance)                                     \
  if (v.size() != 3)  ADD_FAILURE() << "Vector did not have 3 elements"; \
  else { \
  EXPECT_NEAR(a, v[0], tolerance); \
  EXPECT_NEAR(b, v[1], tolerance); \
  EXPECT_NEAR(c, v[2], tolerance); \
  }

#define EXPECT_F3_NEAR(a, b, c, v, tolerance)             \
  EXPECT_NEAR(a, v[0], tolerance); \
  EXPECT_NEAR(b, v[1], tolerance); \
  EXPECT_NEAR(c, v[2], tolerance); 



TEST(MegaTreeBasics, NodeGeometry)
{
  double center[] = {0,0,0};
  NodeGeometry p(center, 10.0);

  NodeGeometry c = p.getChild(3);
  EXPECT_NEAR(5.0, c.getSize(), 1e-3);
  EXPECT_NEAR(-2.5, c.getCenter(0),  1e-6);
  EXPECT_NEAR( 2.5, c.getCenter(1),  1e-6);
  EXPECT_NEAR( 2.5, c.getCenter(2),  1e-6);

  p = NodeGeometry(center, 6386850.000000);
  c = p.getChild(3);
  EXPECT_NEAR(3193425.0, c.getSize(), 1e-3);
  EXPECT_NEAR(-1596712.5, c.getCenter(0),  1e-6);
  EXPECT_NEAR(1596712.5, c.getCenter(1),  1e-6);
  EXPECT_NEAR(1596712.5, c.getCenter(2),  1e-6);

  double new_center[] = {-1596712.500000, 1596712.500000, 1596712.500000};
  p = NodeGeometry(new_center, 3193425.000000);
  c = p.getChild(3);
  EXPECT_NEAR(1596712.5, c.getSize(), 1e-3);
  EXPECT_NEAR(-2395068.75, c.getCenter(0),  1e-6);
  EXPECT_NEAR(2395068.75, c.getCenter(1),  1e-6);
  EXPECT_NEAR(2395068.75, c.getCenter(2),  1e-6);
}


TEST(MegaTreeBasics, NodeGeometryInPlace)
{
  double center[] = {0,0,0};
  NodeGeometry p(center, 10.0);

  NodeGeometry c = p;
  c = c.getChild(3);
  EXPECT_NEAR(5.0, c.getSize(), 1e-3);
  EXPECT_NEAR(-2.5, c.getCenter(0),  1e-6);
  EXPECT_NEAR(2.5, c.getCenter(1),  1e-6);
  EXPECT_NEAR(2.5, c.getCenter(2),  1e-6);
}


TEST(MegaTreeBasics, NodeGeometryPrecision_EdgesShouldAlign)
{
  // Descends 26 levels down, once right, then always left.  The left
  // edge of this small box should match perfectly with the center of
  // the original box.
  double r_lo[] = {-5863017.3200000003, -2192262.1200000001, -6386851.2000000002};
  double r_hi[] = {6910682.6799999997, 10581437.879999999, 6386848.7999999998};
  NodeGeometry r(1, r_lo, r_hi);

  NodeGeometry child = r.getChild(7);  // Descends to the right
  for (size_t i = 0; i < 26; ++i)
  {
    double child_left_edge = child.getLo(0);

    // Note: They must be exactly equal.  If they are only
    // approximately equal a point can be assigned to a particular
    // node, but the node will think that the point is not contained
    // in the node.
    //EXPECT_EQ(r_center[0], child_left_edge) << " at level " << i;
    ASSERT_NEAR(r.getCenter(0), child_left_edge, 0.0) << "Descended " << (i + 1) << " times";

    child = child.getChild(0);  // Descends to the left
  }
}



TEST(MegaTreeBasics, Node)
{
  std::vector<double> tree_center(3,0);
  Node n;

  for (uint8_t i=0; i<8; i++)
    EXPECT_FALSE(n.hasChild(i));

  n.setChild(0);
  EXPECT_TRUE(n.hasChild(0));
  for(uint8_t i = 1; i < 8; ++i)
    EXPECT_FALSE(n.hasChild(i));

  n.setChild(7);
  EXPECT_TRUE(n.hasChild(0));
  EXPECT_TRUE(n.hasChild(7));
  for(uint8_t i = 1; i < 7; ++i)
    EXPECT_FALSE(n.hasChild(i));
}


TEST(MegaTreeBasics, OctreeLevel)
{
  EXPECT_EQ(1, IdType(1).level());
  EXPECT_EQ(2, IdType(010).level());
  EXPECT_EQ(3, IdType(0177).level());
  EXPECT_EQ(8, IdType(014637720).level());

  EXPECT_EQ(IdType(01), IdType(017).getParent());
  EXPECT_EQ(IdType(012747241), IdType(0127472410).getParent());

  EXPECT_EQ(IdType(0), IdType(1).getParent());
}


TEST(MegaTreeBasics, NodePoint)
{
  double point[3], color[3];
  point[0] = point[1] = point[2] = color[0] = color[1] = color[2] = 0;
  point[0] = 0.02;
  color[0] = 230;

  double center[] = {0, 0, 0};
  double tree_size = 1.0;
  NodeGeometry ng(center, tree_size);
  Node n;
  n.setPoint(ng, point, color);
  
  double point_get[3], color_get[3];
  n.getPoint(ng, point_get);
  n.getColor(color_get);
  
  EXPECT_NEAR(point[0], point_get[0], 2.*ng.getSize()/256.);
  EXPECT_NEAR(point[1], point_get[1], 2.*ng.getSize()/256.);
  EXPECT_NEAR(point[2], point_get[2], 2.*ng.getSize()/256.);

  EXPECT_NEAR(color[0], color_get[0], 2.*ng.getSize()/256.);
  EXPECT_NEAR(color[1], color_get[1], 2.*ng.getSize()/256.);
  EXPECT_NEAR(color[2], color_get[2], 2.*ng.getSize()/256.);
}




TEST(MegaTreeBasics, SinglePointRangeQuery)
{
  std::vector<double> tree_center(3, 0);
  double tree_size = 10;
  boost::shared_ptr<TempDir> tree_path = createTempDir("rangequery", true);
  boost::shared_ptr<Storage> storage(openStorage(tree_path->getPath()));
  MegaTree tree(storage, tree_center, tree_size, 3, 1);

  // Adds a single point
  std::vector<double> pt(3, 0);
  addPoint(tree, pt);

  // Queries a range around the point
  std::vector<double> range_lo(3, -1.0f), range_hi(3, 1.0f);
  std::vector<double> result, colors;
  queryRange(tree, range_lo, range_hi, point_precision, result, colors);
  EXPECT_V3_NEAR(0, 0, 0,  result, 2.*point_precision);

  // Queries a range without any points
  range_lo[0] = 1.0f;
  range_lo[1] = 1.0f;
  range_lo[2] = 1.0f;
  range_hi[0] = 4.0f;
  range_hi[1] = 4.0f;
  range_hi[2] = 4.0f;
  result.clear();
  colors.clear();
  queryRange(tree, range_lo, range_hi, point_precision, result, colors);
  EXPECT_EQ(0, result.size());
}


void gridTest(bool clear)
{
  std::vector<double> tree_center(3, 10);
  double tree_size = 30.0;
  NodeGeometry ng(tree_center, tree_size);

  boost::shared_ptr<TempDir> tree_path(createTempDir("rangequery", true));
  boost::shared_ptr<Storage> storage(openStorage(tree_path->getPath()));
  MegaTree tree(storage, tree_center, tree_size, 4, 1);

  // Creates a grid of points
  const double STEP = 1;
  const size_t WIDTH = 20;
  std::vector<double> pt(3, 0.0f);
  for (size_t i = 0; i < WIDTH; ++i)
  {
    for (size_t j = 0; j < WIDTH; ++j)
    {
      for (size_t k = 0; k < WIDTH; ++k)
      {
        pt[0] = STEP * i;
        pt[1] = STEP * j;
        pt[2] = STEP * k;
        addPoint(tree, pt);
      }
    }
  }

  if (clear)
    tree.flushCache();

  // Checks that the correct number of points have been added.
  NodeHandle root;
  tree.getRoot(root);
  ASSERT_TRUE(root.getCount() > 1);  // Probably
  EXPECT_EQ(pow(WIDTH, 3), root.getCount());

  // Checks that the summarization is correct
  double mean = double(WIDTH - 1) / 2 * STEP;
  double point[3];
  root.getPoint(point);
  EXPECT_NEAR(mean, point[0], 2. * tree_size / 256.);
  EXPECT_NEAR(mean, point[1], 2. * tree_size / 256.);
  EXPECT_NEAR(mean, point[2], 2. * tree_size / 256.);
  tree.releaseNode(root);

  // Queries a box with all the points
  std::vector<double> range_lo(3, -1), range_hi(3, WIDTH * STEP + 1);
  std::vector<double> result, colors;
  queryRange(tree, range_lo, range_hi, point_precision, result, colors);
  EXPECT_EQ(pow(WIDTH, 3), result.size() / 3);

  // Queries for half the points
  range_hi[0] = WIDTH * STEP;
  range_hi[1] = WIDTH * STEP;
  range_hi[2] = (WIDTH * STEP) / 2;
  result.clear();
  colors.clear();
  queryRange(tree, range_lo, range_hi, point_precision, result, colors);
  EXPECT_EQ(pow(WIDTH, 3) / 2, result.size() / 3);

  // Queries for exactly 8 points
  range_lo[0] = 2 * STEP - 0.00001;
  range_lo[1] = 2 * STEP - 0.00001;
  range_lo[2] = 2 * STEP - 0.00001;
  range_hi[0] = 4 * STEP - 0.00001;
  range_hi[1] = 4 * STEP - 0.00001;
  range_hi[2] = 4 * STEP - 0.00001;
  result.clear();
  colors.clear();
  queryRange(tree, range_lo, range_hi, point_precision, result, colors);
  EXPECT_EQ(8, result.size() / 3);

  // Queries for a line of points along x
  range_lo[0] = -1000;        range_hi[0] = 1000;
  range_lo[1] = 5.9 * STEP;   range_hi[1] = 6.1 * STEP;
  range_lo[2] = 5.9 * STEP;   range_hi[2] = 6.1 * STEP;
  result.clear();
  colors.clear();
  queryRange(tree, range_lo, range_hi, point_precision, result, colors);
  EXPECT_EQ(WIDTH, result.size() / 3);
}


TEST(MegaTreeBasics, GridRangeQuery)
{
  gridTest(false);
  gridTest(true);
}

TEST(MegaTreeBasics, TestDiskAccess)
{
  std::vector<double> tree_center(3, 0);
  double tree_size = 1000000;

  boost::shared_ptr<TempDir> tree1_path(createTempDir("tree1", true));
  boost::shared_ptr<Storage> storage1(openStorage(tree1_path->getPath()));
  MegaTree tree1(storage1, tree_center, tree_size, 3, 1);

  boost::shared_ptr<TempDir> tree2_path(createTempDir("tree2", true));
  boost::shared_ptr<Storage> storage2(openStorage(tree2_path->getPath()));
  MegaTree tree2(storage2, tree_center, tree_size, 2, 1);

  // Adds a grid of points
  const double STEP = 1;
  const size_t WIDTH = 2;
  std::vector<double> pt(3, 0.0f);
  for (size_t i = 0; i < WIDTH; ++i)
  {
    for (size_t j = 0; j < WIDTH; ++j)
    {
      for (size_t k = 0; k < WIDTH; ++k)
      {
        pt[0] = STEP * i;
        pt[1] = STEP * j;
        pt[2] = STEP * k;
        addPoint(tree1, pt);
        addPoint(tree2, pt);
      }
    }
  }

  tree1.flushCache();
  EXPECT_TRUE(tree1 == tree2);

  //test to make sure we can load from disk
  boost::shared_ptr<Storage> storage3(openStorage(tree1_path->getPath()));
  MegaTree tree3(storage3, 10000, true);

  EXPECT_TRUE(tree1 == tree3);
  EXPECT_TRUE(tree2 == tree3);

}

TEST(MegaTreeBasics, ColorSanityCheck)
{
  std::vector<double> tree_center(3, 0);
  double tree_size = 1000000;

  boost::shared_ptr<TempDir> tree_path(createTempDir("test_color_tree", true));
  boost::shared_ptr<Storage> storage(openStorage(tree_path->getPath()));
  MegaTree tree(storage, tree_center, tree_size, 3, 1);

  // Adds a point with color
  std::vector<double> pt(3, 0);
  std::vector<double> color(3, 0);
  color[0] = 1.0f;
  color[1] = 255.0f;
  addPoint(tree, pt, color);

  NodeHandle root;
  tree.getRoot(root);
  double col[3];
  root.getColor(col);
  EXPECT_NEAR(1.0, col[0], 1e-5);
  EXPECT_NEAR(255.0, col[1], 1e-5);
  tree.releaseNode(root);

  // Writes the tree to disk, and loads it as tree2.
  tree.flushCache();
  boost::shared_ptr<Storage> storage2(openStorage(tree_path->getPath()));
  MegaTree tree2(storage2, 10000, true);
  NodeHandle root2;
  tree2.getRoot(root2);
  root2.getColor(col);
  EXPECT_NEAR(1.0, col[0], 1e-5);
  EXPECT_NEAR(255.0, col[1], 1e-5);
  tree2.releaseNode(root2);
}

TEST(MegaTreeBasics, ColorsAggregateProperly)
{
  std::vector<double> tree_center(3, 0);
  double tree_size = 100;

  boost::shared_ptr<TempDir> tree_path(createTempDir("test_color_tree", true));
  boost::shared_ptr<Storage> storage(openStorage(tree_path->getPath()));
  MegaTree tree(storage, tree_center, tree_size, 2, 1);

  // Add two points with color
  std::vector<double> pt(3, 0);
  std::vector<double> color(3, 0);
  color[0] = 254.0;
  color[1] = 254.0;
  color[2] = 0.0;
  addPoint(tree, pt, color);
  pt[0] = 0.1;
  color[0] = 0.0;
  color[1] = 254.0;
  color[2] = 254.0;
  addPoint(tree, pt, color);

  NodeHandle root;
  tree.getRoot(root);
  double col[3];
  root.getColor(col);
  EXPECT_NEAR(127.0, col[0], 1e-5);
  EXPECT_NEAR(254.0, col[1], 1e-5);
  EXPECT_NEAR(127.0, col[2], 1e-5);
  tree.releaseNode(root);
}


// Run all the tests that were declared with TEST()
int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
