/* Function used to allocate the message buffer. Following 3 steps should be
 * followed by application when needs to send a message.
 * 1. Call allocate_msg_buff to allocate the buffer for a message, the parameter
 * for the function is the length of usr's message length;
 * 2. Call fill_msg_header function to fill the destination of the message
 * 3. Call send_msg to deliver the message
 * */
#include "message.h"
#include "common/include/common.h"
#include "common/include/common_list.h"
#include "message/mock_API/app_info.h"
//#include "message/src/message_router.h"
#include <string.h>

uint32 g_msg_seq = 0;

void check_buff_magic_num(message_t* msg) {
  CHECK(NULL != msg);
  CHECK(NULL != msg->header);
  CHECK(NULL != msg->body);
  CHECK(NULL != msg->tail);
  CHECK(BUFF_MAGIC_HEADER == msg->header->magic_num);
  CHECK(BUFF_MAGIC_TAIL == msg->tail->magic_num);
}


message_t* allocate_msg_buff(uint32 msg_len) {

  message_t* msg = (message_t*)malloc(sizeof(message_t));
  CHECK(NULL != msg);
  memset(msg, 0, sizeof(message_t));
  uint32 buf_len = sizeof(msg_header_t) + msg_len + sizeof(msg_tail_t);
  uint8* msg_buf = (uint8*)malloc(buf_len);
  CHECK(NULL != msg_buf);
  memset(msg_buf, 0, buf_len);

  msg->buf_len = buf_len;
  msg->buf_head = msg_buf;
  msg->buf_ptr = msg_buf;
  msg->header = (msg_header_t*)msg_buf;
  msg->header->msg_len = msg_len;
  msg->header->magic_num = BUFF_MAGIC_HEADER;
  msg->body = msg_buf + sizeof(msg_header_t);
  msg->tail = (msg_tail_t*)(msg_buf + sizeof(msg_header_t) + msg_len);
  msg->tail->magic_num  = BUFF_MAGIC_TAIL;

  INIT_LIST_HEAD(&msg->list);

  return msg;
}



void free_msg_buff(message_t** msg) {
  if(NULL != msg && NULL != *msg) {
#if DBG_MSG
    check_buff_magic_num(*msg);
#endif
    if(NULL != (*msg)->buf_head) {
      free((*msg)->buf_head);
      (*msg)->buf_head = NULL;
      (*msg)->header = NULL;
      (*msg)->body = NULL;
      (*msg)->tail = NULL;
    }
    free(*msg);
    *msg = NULL;
  }
}


message_t* clone_message(message_t* msg) {
  CHECK(NULL != msg);
  message_t* cloned_msg = allocate_msg_buff(msg->header->msg_len);
  CHECK(NULL != cloned_msg);
  memcpy(cloned_msg->buf_head, msg->buf_head, msg->buf_len);
  return cloned_msg;
}

void fill_msg_header(uint8         des_group_id,
                     uint16        des_app_id,
                     uint8         range,
                     int8          msg_priority,
                     uint8         msg_type,
                     uint32        msg_id,
                     message_t*    msg) {
  CHECK(NULL != msg);
  CHECK(NULL != msg->header);
  msg_header_t* header = msg->header;
  header->rcver.group_id = des_group_id;
  header->rcver.app_id = des_app_id;
  header->rcver.role = range;
  header->msg_id = msg_id;
  header->msg_type = msg_type;
  header->priority = msg_priority;
  header->msg_seq = g_msg_seq++;
  header->ack_seq = 0;

  //TODO:fill information about self
  header->snder.group_id = get_self_group_id();
  header->snder.app_id   = get_self_app_id();
  header->snder.role     = get_self_role();
}

void fill_msg_body(void* msg_content, uint32 msg_len, message_t* msg) {
  CHECK(NULL != msg);
  CHECK(NULL != msg->header);
  CHECK(msg->header->msg_len >= msg_len);
  memcpy((void*)msg->body, msg_content, msg_len);
  msg->header->msg_len = msg_len;
}

void print_msg_header(message_t* msg) {
  msg_header_t* mh = msg->header;
  CHECK(NULL != mh);
  printf("Sender: group %d, app %d, range %d\n",
      mh->snder.group_id, mh->snder.app_id, mh->snder.role);
  printf("Receiver: group %d, app %d, range %d\n",
      mh->rcver.group_id, mh->rcver.app_id, mh->rcver.role);
  printf("Msg_type: %d\n", mh->msg_type);
}

void print_msg(message_t* msg) {
  print_msg_header(msg);
  uint32 i;
  for(i = 0; i < msg->header->msg_len; ++i) {
    printf("%c",msg->body[i]);
  }
  printf("\n\tTail:%x\n", msg->tail->magic_num);
}
