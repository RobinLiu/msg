#include "common/base/msg_client.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"

int main(int argc, char** argv) {
  if(argc != 3) {
    printf("Usage: %s dest_group_id  dest_app_id  package_num\n", argv[0]);
    exit(0);
  }

  uint8 group_id = (uint8)atoi(argv[1]);
  uint16 app_id = (uint16)atoi(argv[2]);

  init_sys_info();
  init_msg_system();

  while(1) {

    message_t* msg = NULL;
    uint32 msglen = 2000;
    msg = allocate_msg_buff(msglen);
    CHECK(NULL != msg);

    fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, 1, msg);
    msg->header->msg_type = MSG_TYPE_SYNC_REQ;

    print_msg_header(msg);

    int rsp = 0;
    int ret = send_sync_msg(msg, 10000, &rsp, sizeof(rsp));
    LOG(INFO,"sync msg result is %d, and rsp is %d", ret, rsp);
  }
  return 0;
}
