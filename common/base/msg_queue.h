#ifndef  BASE_MSG_QUEUE_H
#define  BASE_MSG_QUEUE_H

#include "common/include/common.h"
#include "lock.h"
#include <mqueue.h>

#define MSG_QUEUE_BUF_SIZE      2048
#define RETRY_TIMES             4


typedef mqd_t                 msg_queue_id_t;
typedef const char*           queue_identifier_t;

void start_msg_queue(void* thread_data);

msg_queue_id_t get_queue_id();

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

error_no_t send_msg_to_queue(msg_queue_id_t msg_queue_id, message_t* msg, uint8 retry_times);

//msg_queue_id_t get_msg_queue_id(uint8 group_id, uint16 app_id, int32 max_msg_num);

void destroy_msg_queue();

#endif
