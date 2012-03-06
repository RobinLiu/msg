#include "message_center.h"
#include "message_router.h"
#include "common/base/message.h"
#include "common/base/timer.h"
#include "common/base/msg_queue.h"
#include "message/mock_API/app_info.h"
#include "msg_link.h"
#include "mock_driver.h"
#include <unistd.h>
#include <string.h>


void init_msg_center() {
  init_sys_info();
  init_timer_thread();
  init_msg_link();
  create_rcv_thread();
}

void msg_main() 
{
  init_msg_center();
  msg_queue_id_t in_msg_queue = get_msg_center_queue_id();
  LOG(INFO, "Begin receive msg in queue:%d", (int)in_msg_queue);
  uint8 msg_buf[MSG_QUEUE_BUF_SIZE] = {0};
  message_t* msg = NULL;
  unsigned msg_priority;
//  uint32 msg_seq = 0;
  while(1) {
    ssize_t ret = msg_queue_receive(in_msg_queue,
                                   (void*)msg_buf,
                                    sizeof(msg_buf),
                                    &msg_priority);
    if (ret <= 0) {
      LOG(WARNING, "Receive msg from msg queue failed");
      continue;
    }
    LOG(INFO,"Receive msg from APP, length is %d", (int)ret);
    msg_header_t mh;
    memset(&mh, 0, sizeof(mh));
    memcpy(&mh, msg_buf, sizeof(mh));
//    LOG(INFO, "mh->msg_len is %d", mh.msg_len);
    msg = allocate_msg_buff(mh.msg_len);
    CHECK(NULL != msg);
    memcpy(msg->buf_head, msg_buf, ret);
//    msg->header->msg_seq = msg_seq++;
//    print_msg(msg);
    send_msg_to_msg_router(msg);

  }
}
