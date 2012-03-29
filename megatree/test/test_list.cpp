#include <gtest/gtest.h>
#include <cmath>

#include <megatree/list.h>

using namespace megatree;


TEST(MegaTreeList, ListConstruction)
{
  //create a list of ints for this test
  List<int> list;

  EXPECT_FALSE(list.getFrontPointer());
  EXPECT_FALSE(list.getBackPointer());

  list.push_back(1);

  //make sure the list has a front and a back
  EXPECT_EQ(list.back(), 1);
  EXPECT_EQ(list.front(), 1);

  //now, we'll push something onto the front
  list.push_front(2);

  EXPECT_EQ(list.back(), 1);
  EXPECT_EQ(list.front(), 2);

  //now, we'll push something onto the back again
  list.push_back(3);
  EXPECT_EQ(list.back(), 3);
  EXPECT_EQ(list.front(), 2);

  //now, we'll move that something to the front
  list.moveToFront(list.getBackPointer());
  EXPECT_EQ(list.back(), 1);
  EXPECT_EQ(list.front(), 3);

  //now, we'll move that thing to the back
  list.moveToBack(list.getFrontPointer());
  EXPECT_EQ(list.back(), 3);
  EXPECT_EQ(list.front(), 2);

  //and now, move something from the middle of the list
  list.push_back(4);
  list.moveToFront(list.getBackPointer()->previous);
  EXPECT_EQ(list.back(), 4);
  EXPECT_EQ(list.front(), 3);

  //pop things off the list and make sure back and front end up ok
  EXPECT_EQ(list.front(), 3);
  list.pop_front();
  EXPECT_EQ(list.front(), 2);
  list.pop_front();
  EXPECT_EQ(list.front(), 1);
  list.pop_front();

  EXPECT_EQ(list.getFrontPointer(), list.getBackPointer());
  EXPECT_EQ(list.back(), 4);

  //pop the last item of the list and make sure things work
  EXPECT_EQ(list.front(), 4);
  list.pop_front();

  EXPECT_FALSE(list.getFrontPointer());
  EXPECT_FALSE(list.getBackPointer());

  //build a new list to test pop_back
  list.push_front(5);
  list.push_front(6);
  EXPECT_EQ(list.back(), 5);
  EXPECT_EQ(list.front(), 6);

  EXPECT_EQ(list.back(), 5);
  list.pop_back();

  EXPECT_EQ(list.getFrontPointer(), list.getBackPointer());
  EXPECT_EQ(list.front(), 6);
  EXPECT_EQ(list.back(), 6);

  list.pop_back();
  EXPECT_FALSE(list.getFrontPointer());
  EXPECT_FALSE(list.getBackPointer());

}

TEST(MegaTreeList, ListIteration)
{
  //create a list of ints for this test
  List<int> list;

  list.push_back(1);
  list.push_back(2);
  list.push_back(3);
  list.push_back(4);
  list.push_back(5);

  int count = 0;
  for(ListIterator<int> it = list.frontIterator(); !it.finished(); it.next())
  {
    EXPECT_EQ(it.get(), ++count);
  }

  for(ListIterator<int> it = list.backIterator(); !it.finished(); it.previous())
  {
    EXPECT_EQ(it.get(), count--);
  }

}

TEST(MegaTreeList, ListSplice)
{
  //create a list of ints for this test
  List<int> list1, list2;

  list1.push_back(1);
  list1.push_back(2);
  list1.push_back(3);

  list2.push_back(4);
  list2.push_back(5);
  list2.push_back(6);

  list1.spliceToBack(list2);

  int count = 0;
  for(ListIterator<int> it = list1.frontIterator(); !it.finished(); it.next())
  {
    EXPECT_EQ(it.get(), ++count);
  }

  EXPECT_TRUE(list2.empty());

  //test splicing empty lists
  List<int> list3, list4;

  list3.spliceToBack(list4);
  EXPECT_TRUE(list3.empty());
  EXPECT_TRUE(list4.empty());

  //test splicing to an empty list
  list3.spliceToBack(list1);
  count = 0;
  for(ListIterator<int> it = list3.frontIterator(); !it.finished(); it.next())
  {
    EXPECT_EQ(it.get(), ++count);
  }
  EXPECT_TRUE(list1.empty());

  //test splicing an empty list onto the back
  list3.spliceToBack(list4);
  count = 0;
  for(ListIterator<int> it = list3.frontIterator(); !it.finished(); it.next())
  {
    EXPECT_EQ(it.get(), ++count);
  }
  EXPECT_TRUE(list4.empty());


}

// Run all the tests that were declared with TEST()
int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
