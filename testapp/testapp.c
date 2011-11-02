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
    uint32 msg_id = msg->header->msg_id;
    //LOG(INFO, "Received a msg in app id is %d, response with ack", msg->header->msg_id);
    //print_msg_header(msg);
    switch(msg_id) {
    case MSG_ID_TEST_MSG:
      ret++;
      LOG(INFO, "Begin to send rsp msg...");
//      msg->header->msg_seq = 0;
      send_sync_msg_rsp(msg, &ret, sizeof(ret));
      break;
    default:
      LOG(INFO, "Message id %d is received but not processed", msg_id);
      print_msg_header(msg);
      break;
    }

    free_msg_buff(&msg);
  }
  return 0;
}
