#include "msg_queue.h"
#include "thread.h"
#include "message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"
#include <string.h>
#include <errno.h>
#include <time.h>
 #include <unistd.h>

#if USING_POSIX_MSG_QUEUE
#define FILE_MODE         0664
#define QUEUE_NAME_LEN    128
#define MSG_QUEUE_SIZE    64
#define MSG_SEND_TIME     200000000  //In nanoseconds
msg_queue_id_t g_msg_queue_id = -1;
thread_id_t g_receiver_thread_id = 0;

msg_queue_id_t open_msg_queue(queue_identifier_t queue_identifier) {

  struct mq_attr attr;
  attr.mq_maxmsg = MSG_QUEUE_SIZE;
  attr.mq_msgsize = MSG_QUEUE_BUF_SIZE;
  attr.mq_flags = O_RDWR|O_CREAT;
//  mq_unlink(queue_identifier);
  LOG(INFO, "Before set attr.mq_msgsize %ld, attr.mq_maxmsg %ld", attr.mq_msgsize, attr.mq_maxmsg);
  mqd_t mq_id = mq_open(queue_identifier, O_RDWR|O_CREAT, FILE_MODE, &attr);
  CHECK(-1 != mq_id, "%s", strerror(errno));
  LOG(INFO, "%s opened as queue %d", queue_identifier, (int)mq_id);
/*
 *The  values  of  the  mq_maxmsg,  mq_msgsize, and mq_curmsgs members of
 *the mq_attr structure shall be ignored by mq_setattr().
 * */
//  int ret = mq_setattr(mq_id, &attr, NULL);
//  if (ret != 0) {
//    perror("mq_setattr failed");
//  }
  mq_getattr(mq_id, &attr);
  LOG(INFO, "attr.mq_msgsize %ld, attr.mq_maxmsg %ld", attr.mq_msgsize, attr.mq_maxmsg);
  return mq_id;
}

void unlink_msg_queue(queue_identifier_t queue_identifier) {
  int ret = mq_unlink(queue_identifier);
  CHECK(0 == ret);
}


error_no_t msg_queue_send(msg_queue_id_t msg_queue_id,
                          void* msg_data,
                          size_t msg_len,
                          unsigned msg_prio) {

  struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = MSG_SEND_TIME;

  int ret =  mq_timedsend(msg_queue_id,
                          (const char *)msg_data,
                          msg_len,
                          msg_prio,
                          &timeout);
  if(0 != ret) {
//    LOG(ERROR, "Send msg to msg_queue failed: %s", strerror(errno));
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    mq_getattr(msg_queue_id, &attr);
    if (attr.mq_curmsgs == attr.mq_maxmsg) {
      usleep(MSG_SEND_TIME*2/1000);
      ret =  mq_timedsend(msg_queue_id,
                                (const char *)msg_data,
                                msg_len,
                                msg_prio,
                                &timeout);
      if (0 != ret) {
        return SEND_MSG_TO_MSG_QUEUE_FAILED_EC;
      }
    }
//    LOG(ERROR, "attr.mq_curmsgs %ld", attr.mq_curmsgs);
  }

  return SUCCESS_EC;
}


ssize_t msg_queue_receive(msg_queue_id_t msg_queue_id,
                             void* msg_data,
                             size_t msg_len,
                             unsigned *msg_prio) {
  return mq_receive(msg_queue_id, (char*)msg_data, msg_len, msg_prio);
}


msg_queue_id_t get_msg_queue_id(uint8 group_id, uint16 app_id) {
  char queue_name[QUEUE_NAME_LEN] = {0};
  snprintf(queue_name, QUEUE_NAME_LEN, "/group-%d_app-%d", group_id, app_id);
  queue_name[QUEUE_NAME_LEN -1] = '\0';

  LOG(INFO, "Open msg_queue:%s", queue_name);
  msg_queue_id_t msg_queue_id = open_msg_queue(queue_name);

  return msg_queue_id;
}

msg_queue_id_t get_self_msg_queue_id() {
  return get_msg_queue_id(get_self_group_id(), get_self_app_id());
}


msg_queue_id_t get_msg_center_queue_id() {
  return get_msg_queue_id(get_msg_center_group_id(), get_msg_center_app_id());
}


//TODO: get the size form the attribute of the message queue;


void* message_receiver_thread(void* arg) {
  msg_queue_id_t msg_queue_id = get_self_msg_queue_id();
  CHECK(msg_queue_id != -1);
  MSG_RCVD_CB msg_rcvd_cb = (MSG_RCVD_CB)arg;
  char msg_buf[MSG_QUEUE_BUF_SIZE] = {0};
  unsigned msg_prio = 0;
  while (1) {
    memset(msg_buf, 0, MSG_QUEUE_BUF_SIZE);
    ssize_t ret = msg_queue_receive(msg_queue_id, msg_buf, MSG_QUEUE_BUF_SIZE, &msg_prio);
    if(ret <0 ) {
      perror("receive msg in msg queue failed");
    }
    msg_header_t mh;
    memset(&mh, 0, sizeof(mh));
    memcpy(&mh, msg_buf, sizeof(mh));
    message_t* msg = allocate_msg_buff(mh.msg_len);
    memcpy(msg->buf_head, msg_buf, ret);
    (*msg_rcvd_cb)((void*)msg);
    free_msg_buff(&msg);
  }
}



error_no_t send_msg(message_t* msg) {
  return msg_queue_send(g_msg_queue_id,
                        (void*)msg->buf_head,
                        (size_t)msg->buf_len,
                        msg->header->priority);
}

error_no_t send_msg_to_queue(msg_queue_id_t msg_queue_id, message_t* msg) {
  return msg_queue_send(msg_queue_id,
                        (void*)msg->buf_head,
                        (size_t)msg->buf_len,
                        msg->header->priority);
}


void init_msg_queue(MSG_RCVD_CB msg_rcvd_cb) {
  g_msg_queue_id = get_msg_center_queue_id();


  create_thread(&g_receiver_thread_id,
                NULL,
                message_receiver_thread,
                (void*)msg_rcvd_cb);
}

void destroy_msg_queue() {
  mq_close(g_msg_queue_id);
  cancel_thread(g_receiver_thread_id);

}
#endif

