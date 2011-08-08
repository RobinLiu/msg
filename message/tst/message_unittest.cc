#include <limits.h>
#include "gtest/gtest.h"
#include "message.h"

TEST(ALLOC_MSG_TEST, normal_alloc) {
  message_t* msg = NULL;
  msg = allocate_msg_buff(10);
  ASSERT_TRUE(NULL != msg);
  ASSERT_TRUE(NULL != msg->header);
  ASSERT_TRUE(NULL != msg->body);
  ASSERT_TRUE(NULL != msg->tail);
  ASSERT_TRUE(NULL != msg->buf_head);
  ASSERT_TRUE(NULL != msg->buf_ptr);
  EXPECT_EQ(10, msg->header->msg_len);
}

TEST(ALLOC_MSG_TEST, alloc_zero_msg) {
  message_t* msg = NULL;
  msg = allocate_msg_buff(0);
  ASSERT_TRUE(NULL != msg);
  EXPECT_EQ(0, msg->header->msg_len);
}

TEST(DEL_MSG_TEST, normal_del) {
  message_t* msg = NULL;
  msg = allocate_msg_buff(10);
  free_msg_buff(&msg);
  ASSERT_TRUE(msg == NULL);
}

TEST(DEL_MSG_TEST, del_zeor_msg) {
  message_t* msg = NULL;
  msg = allocate_msg_buff(0);
  free_msg_buff(&msg);
  ASSERT_TRUE(msg == NULL);
}

TEST(DEL_MSG_TEST, del_NULL_msg) {
  message_t* msg = NULL;
  free_msg_buff(&msg);
  ASSERT_TRUE(msg == NULL);
}
