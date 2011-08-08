#ifndef MESSAGE_ROUTER_H
#define MESSAGE_ROUTER_H
#ifdef __cplusplus
extern "C" {
#endif
#include "common/include/common.h"
//#include "message.h"




error_no_t send_msg_to_msg_router(message_t* msg);

error_no_t router_receive_msg(message_t* msg);

#ifdef __cplusplus
}
#endif
#endif
