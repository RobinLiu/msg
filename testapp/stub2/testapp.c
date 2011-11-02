#include "common/base/msg_client.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/mock_API/app_info.h"

int main(int argc, char** argv) {
  if(argc != 3) {
    printf("Usage: %s dest_group_id  dest_app_id  package_num\n", argv[0]);
    exit(0);
  }

  uint8 dst_grp_id = (uint8)atoi(argv[1]);
  uint16 dst_app_id = (uint16)atoi(argv[2]);
  int msg_content = 0;
  int rsp = 0;
  int8 msg_priority = 1;
  uint32 time_out = 10000;
  uint32 msg_id = 1;

  init_sys_info();
  init_msg_system();

  while(1) {
    int ret = send_sync_msg(dst_grp_id,
                            dst_app_id,
                            msg_id,
                            msg_priority,
                            &msg_content,
                            sizeof(msg_content),
                            time_out,
                            &rsp,
                            sizeof(rsp));
    msg_content++;
    LOG(INFO,"sync msg result is %d, and rsp is %d", ret, rsp);
    ret = send_msg(dst_grp_id,
                   dst_app_id,
                   2,
                   msg_priority,
                   &msg_content,
                   sizeof(msg_content));
  }
  return 0;
}
