#include "msg_queue.h"
#include "thread.h"
#include "message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"
#include "common/include/common_list.h"

#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
 #include <poll.h>

#define FILE_MODE               0664
#define QUEUE_NAME_LEN          128
#define CLIENT_MSG_QUEUE_SIZE   64
#define SERVER_MSG_QUEUE_SIZE   512
#define MSG_SEND_TIME           500000000  //0.5s In nanoseconds

typedef struct {
  struct list_head list;
  message_t* msg_req;
  message_t* msg_rsp;
  int fd[2];
} sync_msg_pair_t;


msg_queue_id_t g_msg_queue_id = -1;
thread_id_t g_receiver_thread_id = 0;
int msg_seq = 0;

void init_msg_client(msg_client_t* msg_client) {

  CHECK(NULL != msg_client);
  INIT_LIST_HEAD(&msg_client->sync_msg_pair_list);
  INIT_LIST_HEAD(&msg_client->aync_msg_list);
  init_lock(&msg_client->sync_msg_list_lock);
  init_lock(&msg_client->async_msg_list_lock);
  CHECK(0 == pipe(msg_client->fd));
}


msg_queue_id_t open_msg_queue(queue_identifier_t queue_identifier,
                              int32 max_msg_num) {

  struct mq_attr attr;
  attr.mq_maxmsg = max_msg_num;
  attr.mq_msgsize = MSG_QUEUE_BUF_SIZE;
  attr.mq_flags = O_RDWR|O_CREAT;
//  mq_unlink(queue_identifier);
  LOG(INFO, "Before set attr.mq_msgsize %ld, attr.mq_maxmsg %ld",
      attr.mq_msgsize, attr.mq_maxmsg);
  mqd_t mq_id = mq_open(queue_identifier, O_RDWR|O_CREAT, FILE_MODE, &attr);
  CHECK(-1 != mq_id, "%s", strerror(errno));
  LOG(INFO, "%s opened as queue %d", queue_identifier, (int)mq_id);
  mq_getattr(mq_id, &attr);
  LOG(INFO, "attr.mq_msgsize %ld, attr.mq_maxmsg %ld",
      attr.mq_msgsize, attr.mq_maxmsg);
  return mq_id;
}

void unlink_msg_queue(queue_identifier_t queue_identifier) {
  int ret = mq_unlink(queue_identifier);
  CHECK(0 == ret);
}


error_no_t msg_queue_send(msg_queue_id_t msg_queue_id,
                          void* msg_data,
                          size_t msg_len,
                          unsigned msg_prio,
                          uint8 retry_times) {

#if USE_TIMED_SEND
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
      while(0 != retry_times-- && 0 != ret) {
//      usleep(MSG_SEND_TIME/1000);
      ret =  mq_timedsend(msg_queue_id,
                                (const char *)msg_data,
                                msg_len,
                                msg_prio,
                                &timeout);
      }
    }
    if (0 != ret) {
      LOG(ERROR, "Send msg failed attr.mq_curmsgs %ld:%s",
          attr.mq_curmsgs, strerror(errno));
//      exit(0);
      return SEND_MSG_TO_MSG_QUEUE_FAILED_EC;
    }
//    LOG(ERROR, "attr.mq_curmsgs %ld", attr.mq_curmsgs);
  }
  return SUCCESS_EC;
#else
  (void)retry_times;
  return mq_send(msg_queue_id, (const char *)msg_data, msg_len, msg_prio);
#endif
}


ssize_t msg_queue_receive(msg_queue_id_t msg_queue_id,
                             void* msg_data,
                             size_t msg_len,
                             unsigned *msg_prio) {
  return mq_receive(msg_queue_id, (char*)msg_data, msg_len, msg_prio);
}


msg_queue_id_t get_msg_queue_id(uint8 group_id, uint16 app_id,
                                int32 max_msg_num) {
  char queue_name[QUEUE_NAME_LEN] = {0};
  snprintf(queue_name, QUEUE_NAME_LEN, "/group-%d_app-%d", group_id, app_id);
  queue_name[QUEUE_NAME_LEN -1] = '\0';

  LOG(INFO, "Open msg_queue:%s", queue_name);
  msg_queue_id_t msg_queue_id = open_msg_queue(queue_name, max_msg_num);

  return msg_queue_id;
}

msg_queue_id_t get_client_msg_queue_id(uint8 group_id, uint16 app_id) {
  return get_msg_queue_id(group_id, app_id, CLIENT_MSG_QUEUE_SIZE);
}

msg_queue_id_t get_self_msg_queue_id() {
  return get_msg_queue_id(get_self_group_id(),
                          get_self_app_id(),
                          CLIENT_MSG_QUEUE_SIZE);
}


msg_queue_id_t get_msg_center_queue_id() {
  return get_msg_queue_id(get_msg_center_group_id(),
                          get_msg_center_app_id(),
                          SERVER_MSG_QUEUE_SIZE);
}

//int is_same_msg_header(message_t* msg_a, message_t* msg_b) {
//  CHECK(NULL != msg_a);
//  CHECK(NULL != msg_b);
//  int is_same_sender = memcmp(&msg_a->header->snder, &msg_a->header->snder, sizeof(msg_sender_t));
//  int is_same_receiver = memcmp(&msg_a->header->rcver, &msg_a->header->rcver, sizeof(msg_receiver_t));
//  int is_same_msg_seq = (msg_a->header->msg_seq ==  msg_b->header->msg_seq);
//  return (is_same_sender && is_same_receiver && is_same_msg_seq);
//}

int is_correct_rsp_msg(message_t* msg_req, message_t* msg_rsp) {
  CHECK(NULL != msg_req);
  CHECK(NULL != msg_rsp);
  int is_same_sender = msg_req->header->snder.group_id == msg_rsp->header->rcver.group_id &&
      msg_req->header->snder.app_id == msg_rsp->header->rcver.app_id;
  int is_same_receiver = msg_req->header->rcver.group_id == msg_rsp->header->snder.group_id &&
      msg_req->header->rcver.app_id == msg_rsp->header->snder.app_id;
  int is_same_msg_seq = (msg_req->header->msg_seq == msg_rsp->header->msg_seq);
  LOG(INFO, "is_same_sender %d, is_same_receiver %d, is_same_msg_seq %d",
      is_same_sender, is_same_receiver, is_same_msg_seq);
  LOG(INFO, "msg_req->header->msg_seq %d, msg_rsp->header->msg_seq %d",
      msg_req->header->msg_seq, msg_rsp->header->msg_seq);
  //TODO:
  is_same_msg_seq = 1;
  return (is_same_sender && is_same_receiver && is_same_msg_seq);
}

void notify_sync_msg_caller(sync_msg_pair_t* sync_msg_pair) {
  CHECK(NULL != sync_msg_pair);
  CHECK(1 == write(sync_msg_pair->fd[1], "0", 1));
}

//sync message response received, notify the caller.
void put_rsp_msg_to_sync_queue(message_t* msg, msg_client_t* msg_client) {
  CHECK(NULL != msg);
  CHECK(NULL != msg_client);

  struct list_head* plist = NULL;
  sync_msg_pair_t* iter = NULL;
  bool bReqfound = FALSE;
  lock(&msg_client->sync_msg_list_lock);
  list_for_each(plist, &msg_client->sync_msg_pair_list) {
    LOG(INFO, "checking...");
    iter = list_entry(plist, sync_msg_pair_t, list);
    CHECK(NULL != iter);
    if (is_correct_rsp_msg(iter->msg_req, msg)) {
      //notify the caller of the sync message.
      iter->msg_rsp = msg;
      bReqfound = TRUE;
      notify_sync_msg_caller(iter);
      LOG(INFO, "Correct sync msg received, notify the caller");
      break;
    }
  }

   if (!bReqfound) {
      //If there's no original req found, there are two possibilties:
      //1. The orignal call timed out and remove the req from the queue;
      //2. A wrong message is received.
      // in those cases, we just throw away the message and write a log.
      LOG(WARNING, "A sync response message is received, but the caller timed out.");
      free_msg_buff(&msg);
    }
  unlock(&msg_client->sync_msg_list_lock);
}

void  notify_msg_reader(msg_client_t* msg_client) {
  CHECK(NULL != msg_client);
  CHECK(1 == write(msg_client->fd[1], "0", 1));
}

void put_msg_to_async_queue(message_t* msg, msg_client_t* msg_client) {
  CHECK(NULL != msg);
  CHECK(NULL != msg_client);
  lock(&msg_client->async_msg_list_lock);
  bool was_empty = list_empty(&msg_client->aync_msg_list);
  list_add_tail(&msg_client->aync_msg_list, &msg->list);
  if(was_empty) {
    notify_msg_reader(msg_client);
  }
  unlock(&msg_client->async_msg_list_lock);
}

int get_first_msg(msg_client_t* msg_client, message_t** msg) {
  CHECK(NULL != msg_client);
  CHECK(NULL != msg);

  int num_of_msg_got = 0;
  message_t* receive_msg = NULL;
  lock(&msg_client->async_msg_list_lock);
  bool is_empty = list_empty(&msg_client->aync_msg_list);
  LOG(INFO, "is_empty %d", is_empty);
  if(!is_empty) {
    receive_msg = list_entry(msg_client->aync_msg_list.next, message_t, list);
    CHECK(NULL != receive_msg);
    list_del(&receive_msg->list);
    *msg = receive_msg;
    num_of_msg_got = 1;
  }
  unlock(&msg_client->async_msg_list_lock);
  return num_of_msg_got;
}

int receive_msg(msg_client_t* msg_client, message_t** msg) {
  CHECK(NULL != msg_client);
  CHECK(NULL != msg);
  int num_of_msg_received = 0;
  int time_out = -1; //milliseconds;
  num_of_msg_received = get_first_msg(msg_client, msg);
  if(1 == num_of_msg_received) {
    return num_of_msg_received;
  }

  struct pollfd pfd = {msg_client->fd[0], POLLIN, 0};
  char flag[1] = {0};
  int ret = poll(&pfd, 1, time_out);
  if (0 == ret) {
    LOG(INFO, "timeout, read msg from async queue...");
    num_of_msg_received = get_first_msg(msg_client, msg);
  } else if (1 ==ret) {
    LOG(INFO, "Notified, read msg from async queue...");
    CHECK(1 == read(msg_client->fd[0], flag, 1));
    num_of_msg_received = get_first_msg(msg_client, msg);
  } else {
    LOG(INFO, "%s", strerror(errno));
  }

  return num_of_msg_received;
}

void* message_receiver_thread(void* arg) {
  msg_queue_id_t msg_queue_id = get_self_msg_queue_id();
  CHECK(msg_queue_id != -1);
  (void)arg;
//  MSG_RCVD_CB msg_rcvd_cb = (MSG_RCVD_CB)arg;
  msg_client_t* msg_client = (msg_client_t*)arg;
  char msg_buf[MSG_QUEUE_BUF_SIZE] = {0};
  unsigned msg_prio = 0;
  while (1) {
    memset(msg_buf, 0, MSG_QUEUE_BUF_SIZE);
    ssize_t ret = msg_queue_receive(msg_queue_id,
                                    msg_buf,
                                    MSG_QUEUE_BUF_SIZE,
                                    &msg_prio);
    if(ret <0 ) {
      LOG(ERROR, "receive msg in msg queue failed: %s", strerror(errno));
    }
    LOG(INFO, "Received a msg form mq");
    msg_header_t mh;
    memset(&mh, 0, sizeof(mh));
    memcpy(&mh, msg_buf, sizeof(mh));
    message_t* msg = allocate_msg_buff(mh.msg_len);
    memcpy(msg->buf_head, msg_buf, ret);
    print_msg_header(msg);
    switch(mh.msg_type) {
    case MSG_TYPE_SYNC_RSP:
//      LOG(INFO, "Receive a sync req msg");
      put_rsp_msg_to_sync_queue(msg, msg_client);
      break;
    case MSG_TYPE_SYNC_REQ:
    case MSG_TYPE_ASYNC_REQ:
    case MSG_TYPE_ASYNC_RSP:
      LOG(INFO, "Receive a normal msg");
      put_msg_to_async_queue(msg, msg_client);
      break;
    default:
      LOG(INFO, "Receive a unknown msg");
      break;
    }
    //if(MSG_TYPE_SYNC_RSP)
    //decide which queues the msg goes
//    (*msg_rcvd_cb)((void*)msg);
//    free_msg_buff(&msg);
  }
}



error_no_t send_msg(message_t* msg) {

  msg->header->msg_seq = msg_seq++;
  return msg_queue_send(g_msg_queue_id,
                        (void*)msg->buf_head,
                        (size_t)msg->buf_len,
                        msg->header->priority,
                        RETRY_TIMES);
}

error_no_t send_msg_to_queue(msg_queue_id_t msg_queue_id,
                             message_t* msg,
                             uint8 retry_times) {
  return msg_queue_send(msg_queue_id,
                        (void*)msg->buf_head,
                        (size_t)msg->buf_len,
                        msg->header->priority,
                        retry_times);
}


//void init_msg_system(MSG_RCVD_CB msg_rcvd_cb) {
void init_msg_system(msg_client_t* msg_client) {
  g_msg_queue_id = get_msg_center_queue_id();
  init_msg_client(msg_client);

  create_thread(&g_receiver_thread_id,
                NULL,
                message_receiver_thread,
                (void*)msg_client);
}

void destroy_msg_queue() {
  mq_close(g_msg_queue_id);
  cancel_thread(g_receiver_thread_id);
}

void enqueue_msg_to_list(sync_msg_pair_t* sync_msg_pair, msg_client_t* msg_client) {
  CHECK(NULL != sync_msg_pair);
  CHECK(NULL != msg_client);
  lock(&msg_client->sync_msg_list_lock);
  list_add_tail(&msg_client->sync_msg_pair_list, &sync_msg_pair->list);
  unlock(&msg_client->sync_msg_list_lock);
}


int wait_for_rsp(int time_out, sync_msg_pair_t* sync_msg_pair) {

  CHECK(NULL != sync_msg_pair);
  struct pollfd pfd = {sync_msg_pair->fd[0], POLLIN, 0};
  char flag[1] = {0};
  int ret = poll(&pfd, 1, time_out);
  if (0 == ret) {
    LOG(INFO, "Poll time out");
  } else if (1 ==ret) {
    CHECK(1 == read(sync_msg_pair->fd[0], flag, 1));
  } else {
    LOG(INFO, "%s", strerror(errno));
  }
  return ret;
}


int send_sync_msg(message_t* msg, int time_out, msg_client_t* msg_client) {

  sync_msg_pair_t* sync_msg_pair = (sync_msg_pair_t*)malloc(sizeof(sync_msg_pair_t));
  CHECK(NULL != sync_msg_pair);
  sync_msg_pair->msg_req  = msg;
  sync_msg_pair->msg_rsp = NULL;
  INIT_LIST_HEAD(&sync_msg_pair->list);
  if (0 != pipe(sync_msg_pair->fd)) {
    LOG(ERROR, "pipe failed: %s", strerror(errno));
    free(sync_msg_pair);
    sync_msg_pair = NULL;
    return errno;
  }

  if (0 != send_msg(msg)) {
    LOG(WARNING, "Send message failed");
    free(sync_msg_pair);
    sync_msg_pair = NULL;
    return SEND_MSG_TO_MSG_QUEUE_FAILED_EC;
  }

  LOG(INFO, "Send msg to mq done");
  enqueue_msg_to_list(sync_msg_pair, msg_client);

  wait_for_rsp(time_out, sync_msg_pair);

  int remote_rsp = -1;
  if(NULL != sync_msg_pair->msg_rsp) {
    memcpy(&remote_rsp, sync_msg_pair->msg_rsp->body, sizeof(int));
  }
  LOG(INFO, "Response from remote is %d", remote_rsp);

  //destroy the message node;
  lock(&msg_client->sync_msg_list_lock);
  list_del(&sync_msg_pair->list);
  free_msg_buff(&sync_msg_pair->msg_req);
  if(NULL != sync_msg_pair->msg_rsp) {
    free_msg_buff(&sync_msg_pair->msg_rsp);
  }
  free(sync_msg_pair);
  unlock(&msg_client->sync_msg_list_lock);

  return remote_rsp;
}

error_no_t send_rsp_msg(message_t* msg, int msg_id, void* rsp_buf, int buf_size) {
  CHECK(NULL != msg);
  CHECK(NULL != rsp_buf);
  CHECK(buf_size > 0);
  LOG(INFO, "in send_rsp_msg");
  message_t* rsp_msg =  allocate_msg_buff(buf_size);
  CHECK(NULL != rsp_msg);
  memcpy(&rsp_msg->header->rcver, &msg->header->snder, sizeof(msg_receiver_t));
  fill_msg_header(msg->header->snder.group_id,
                  msg->header->snder.app_id,
                  msg->header->snder.role,
                  msg->header->priority,
                  msg_id,
                  rsp_msg);
  //TODO:
  rsp_msg->header->msg_type = MSG_TYPE_SYNC_RSP;
  memcpy(rsp_msg->body, rsp_buf, buf_size);
  LOG(INFO, "RSP msg sent");
  print_msg_header(rsp_msg);
  return send_msg(rsp_msg);
}
