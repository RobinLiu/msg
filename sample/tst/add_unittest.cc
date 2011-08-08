
#include <limits.h>
#include "add.h"
#include "gtest/gtest.h"

TEST(ADD_TEST, add_1) {
   
EXPECT_EQ(2, add(1,1));
//EXPECT_EQ(2, add(1,2));
}
