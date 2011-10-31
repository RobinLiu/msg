#ifndef  BASE_MSG_QUEUE_H
#define  BASE_MSG_QUEUE_H

#include "common/include/common.h"
#include "lock.h"
#define MSG_QUEUE_BUF_SIZE      2048
#define RETRY_TIMES             4

#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K  //MACRO for mq_timedsend
#endif
#include <mqueue.h>
typedef mqd_t                 msg_queue_id_t;
typedef const char*           queue_identifier_t;
//typedef void (*MSG_RCVD_CB)(void *);

typedef struct msg_client_t {
  struct list_head sync_msg_pair_list;
  struct list_head aync_msg_list;
  lock_t sync_msg_list_lock;
  lock_t async_msg_list_lock;
  int fd[2]; //pipe used for notification between threads.
} msg_client_t;

//msg_queue_id_t open_msg_queue(queue_identifier_t queue_identifier, int32 max_msg_num);
msg_queue_id_t get_msg_center_queue_id();

msg_queue_id_t get_client_msg_queue_id(uint8 group_id, uint16 app_id);

void unlink_msg_queue(queue_identifier_t queue_identifier);

error_no_t msg_queue_send(msg_queue_id_t msg_queue_id,
                          void* msg_data,
                          size_t msg_len,
                          unsigned msg_prio,
                          uint8 retry_times);

error_no_t msg_queue_receive(msg_queue_id_t msg_queue_id,
                             void* msg_data,
                             size_t msg_len,
                             unsigned int *msg_prio);

msg_queue_id_t get_self_msg_queue_id();

//void init_msg_system(MSG_RCVD_CB msg_rcvd_cb);
void init_msg_system(msg_client_t* msg_client);

error_no_t send_msg(message_t* msg);

error_no_t send_msg_to_queue(msg_queue_id_t msg_queue_id, message_t* msg, uint8 retry_times);

//msg_queue_id_t get_msg_queue_id(uint8 group_id, uint16 app_id, int32 max_msg_num);

void destroy_msg_queue();

error_no_t send_sync_msg(message_t* msg, int time_out, msg_client_t* msg_client);

int receive_msg(msg_client_t* msg_client, message_t** msg);

error_no_t send_rsp_msg(message_t* msg, int msg_id, void* rsp_buf, int buf_size);
#endif
