#include "common/include/common.h"
#include "message/client/app_config.h"
#include "message_center.h"
#include <unistd.h>

int main(int argc, char** argv)
{
  if(argc != 2)
  {
      printf("Usage age %s [Node Id] \n", argv[0]);
      exit(0);
  }
  uint8 node_id = (uint8)atoi(argv[1]);
  if(node_id >= MAX_NODE_NUM)
  {
      printf("Node Id should betwwen [0 - %d) \n", MAX_NODE_NUM);
      exit(0);
  }

  init_msg_center(node_id);
  msg_main();

  LOG(INFO, "Exit");
  return 0;
}
