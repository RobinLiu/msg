#include "common/base/msg_client.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"

#define MSG_ID_TEST_MSG 1

int main() {

  init_sys_info();
  init_msg_system();
  message_t* msg = NULL;
  int ret = 1;
  while(1) {
    receive_msg(&msg);
    LOG(INFO, "Received a msg in app id is %d, response with ack", msg->header->msg_id);
    print_msg_header(msg);
    if(msg->header->msg_id == MSG_ID_TEST_MSG) {
      ret++;
      LOG(INFO, "Begin to send rsp msg...");
      send_sync_rsp_msg(msg, MSG_ID_TEST_MSG, &ret, sizeof(ret));
    }
    free_msg_buff(&msg);
  }
  return 0;
}
