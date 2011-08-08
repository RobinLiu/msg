#ifndef  BASE_MSG_QUEUE_H
#define  BASE_MSG_QUEUE_H

#include "common/include/common.h"

#if USING_POSIX_MSG_QUEUE
#define MSG_QUEUE_BUF_SIZE      2048
#define __USE_XOPEN2K  //MACRO for mq_timedsend
#include <mqueue.h>
typedef mqd_t                 msg_queue_id_t;
typedef const char*           queue_identifier_t;
typedef void (*MSG_RCVD_CB)(void *);

msg_queue_id_t open_msg_queue(queue_identifier_t queue_identifier);

void unlink_msg_queue(queue_identifier_t queue_identifier);

error_no_t msg_queue_send(msg_queue_id_t msg_queue_id,
                          void* msg_data,
                          size_t msg_len,
                          unsigned msg_prio);

error_no_t msg_queue_receive(msg_queue_id_t msg_queue_id,
                             void* msg_data,
                             size_t msg_len,
                             unsigned int *msg_prio);

msg_queue_id_t get_self_msg_queue_id();

void init_msg_queue(MSG_RCVD_CB msg_rcvd_cb);

error_no_t send_msg(message_t* msg);

error_no_t send_msg_to_queue(msg_queue_id_t msg_queue_id, message_t* msg);

msg_queue_id_t get_msg_queue_id(uint8 group_id, uint16 app_id);

void destroy_msg_queue();

#endif


#endif
