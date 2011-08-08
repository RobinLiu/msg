#include "common/include/common.h"
#include "message_center.h"
#include <unistd.h>

int main()
{
  msg_main();
  LOG(INFO, "Exit");
  return 0;
}
