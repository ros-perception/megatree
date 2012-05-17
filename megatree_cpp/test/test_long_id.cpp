#include <gtest/gtest.h>
#include <cmath>

#include <megatree/long_id.h>
#include <megatree/storage_factory.h>
#include <megatree/tree_functions.h>

using namespace megatree;


TEST(LongId, TreeLevel)
{
  EXPECT_EQ(0, 0);
  EXPECT_EQ(0, IdType(0).level());
  EXPECT_EQ(1, IdType(01).level());
  EXPECT_EQ(2, IdType(012).level());
  EXPECT_EQ(3, IdType(0123).level());
  EXPECT_EQ(4, IdType(01234).level());

  IdType id(1);
  EXPECT_TRUE(id.isRoot());
  EXPECT_EQ(1, id.level());
  EXPECT_TRUE(id.isValid());

  EXPECT_EQ(IdType(0).level(), 0);
  EXPECT_EQ(IdType(0).level(), 0);
  EXPECT_EQ(IdType(0), id.getParent());
  EXPECT_EQ(0, id.getParent().level());
  EXPECT_EQ(IdType(014), id.getChild(4));  
  EXPECT_EQ(IdType(01572), id.getChild(5).getChild(7).getChild(2));

  id = id.getChild(5).getChild(7).getChild(2);
  EXPECT_EQ(4, id.level());
  EXPECT_EQ(3, id.getParent().level());
  EXPECT_EQ(0, id.getBit(X_BIT));
  EXPECT_EQ(0, id.getBit(Z_BIT));
  EXPECT_EQ(1, id.getBit(Y_BIT));

  EXPECT_EQ(id.toString(), "01572");
  EXPECT_EQ(IdType(0).toString(), "0");
  EXPECT_EQ(IdType().toString(), "0");
}


TEST(LongId, Subtree)
{
  IdType root, id;
  ShortId short_id;
  std::vector<double> center(3, 0);
  boost::shared_ptr<TempDir> tree_path(createTempDir("subtree", true));
  double tree_size = 10.0;
  unsigned subtree_width = 5;
  unsigned subfolder_depth = 8;
  boost::shared_ptr<Storage> storage(openStorage(tree_path->getPath()));
  MegaTree tree(storage, center, tree_size, subtree_width, subfolder_depth, 10000000);

  id = IdType(01);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(1, id.level());
  EXPECT_EQ(0, root.level());
  EXPECT_EQ(1, short_id);
  EXPECT_EQ("0", root.toString());

  id = IdType(010);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(2, id.level());
  EXPECT_EQ(0, root.level());
  EXPECT_EQ(1*8+0, short_id);
  EXPECT_EQ("0", root.toString());

  id = IdType(0100);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(3, id.level());
  EXPECT_EQ(0, root.level());
  EXPECT_EQ(1*8*8+0*8+0, short_id);
  EXPECT_EQ("0", root.toString());

  id = IdType(01603);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(4, id.level());
  EXPECT_EQ(0, root.level());
  EXPECT_EQ(1*8*8*8+6*8*8+0*8+3, short_id);
  EXPECT_EQ("0", root.toString());

  id = IdType(016066);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(5, id.level());
  EXPECT_EQ(0, root.level());
  EXPECT_EQ(1*8*8*8*8+6*8*8*8+0*8*8+6*8+6, short_id);
  EXPECT_EQ("0", root.toString());

  id = IdType(0166333);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(6, id.level());
  EXPECT_EQ(1, root.level());
  EXPECT_EQ(6*8*8*8*8+6*8*8*8+3*8*8+3*8+3, short_id);
  EXPECT_EQ("01", root.toString());

  id = IdType(01663337);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(7, id.level());
  EXPECT_EQ(2, root.level());
  EXPECT_EQ(6*8*8*8*8+3*8*8*8+3*8*8+3*8+7, short_id);
  EXPECT_EQ("016", root.toString());

  id = IdType(016633366);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(8, id.level());
  EXPECT_EQ(3, root.level());
  EXPECT_EQ(3*8*8*8*8+3*8*8*8+3*8*8+6*8+6, short_id);
  EXPECT_EQ("0166", root.toString());

  id = IdType(0166333333);
  root = tree.getFileId(id);
  short_id = tree.getShortId(id);
  EXPECT_EQ(9, id.level());
  EXPECT_EQ(4, root.level());
  EXPECT_EQ(3*8*8*8*8+3*8*8*8+3*8*8+3*8+3, short_id);
  EXPECT_EQ("01663", root.toString());
}


// Run all the tests that were declared with TEST()
int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
