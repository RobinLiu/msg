#include "gtest/gtest.h"
#include "common_list.h"


struct msg_frag {
  int a;
  int b;
  list_head list;
};

TEST(COMMON_LIST_TEST, list_init) {
  LIST_HEAD(head);
  EXPECT_EQ(&head, head.next);
  EXPECT_EQ(&head, head.prev);
  ASSERT_TRUE(list_empty(&head));
}

TEST(COMMON_LIST_TEST, list_add_and_del) {
  LIST_HEAD(msg_queue);
  struct msg_frag* newlist = (struct msg_frag*)malloc(sizeof(struct msg_frag));
  newlist->a = 1;
  newlist->b = 2;
  list_add(&newlist->list, &msg_queue);
  EXPECT_EQ(msg_queue.next, &newlist->list);
  struct msg_frag* entry = list_entry(&newlist->list, struct msg_frag, list);
  EXPECT_EQ(newlist, entry);
  entry = list_entry(msg_queue.next, struct msg_frag, list);
  EXPECT_EQ(newlist, entry);
  EXPECT_EQ(1, entry->a);
  EXPECT_EQ(2, entry->b);

//  list_del(msg_queue.next);
  list_del(&newlist->list);
  EXPECT_EQ(1, newlist->a);
  EXPECT_EQ(2, newlist->b);
  EXPECT_EQ(NULL, newlist->list.next);
  EXPECT_EQ(NULL, newlist->list.prev);
  ASSERT_TRUE(list_empty(&msg_queue));
}
