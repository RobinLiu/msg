#ifndef BASE_MSG_CLIENT_H
#define BASE_MSG_CLIENT_H
#include "common/include/common.h"
#include "lock.h"

typedef struct msg_client_t {
  struct list_head sync_msg_pair_list;
  struct list_head aync_msg_list;
  lock_t sync_msg_list_lock;
  lock_t async_msg_list_lock;
  int fd[2]; //pipe used for notification between threads.
} msg_client_t;

void init_msg_system();

error_no_t send_msg(message_t* msg);

void receive_msg(message_t** msg);

error_no_t send_sync_msg(message_t* msg, int time_out, void* rsp_buf, uint32 rsp_buf_size);

error_no_t send_sync_rsp_msg(message_t* msg, int msg_id, void* rsp_buf, int rsp_buf_size);

#endif
