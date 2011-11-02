#ifndef MESSAGE_SYSTEM_MESSAGE_H
#define MESSAGE_SYSTEM_MESSAGE_H
#include "common/include/common.h"
//#include "common/include/common_list.h"

#ifdef __cplusplus
extern "C" {
#endif

message_t* allocate_msg_buff(uint32 msg_len);

void check_buff_magic_num(message_t* msg);

void fill_msg_header(uint8   des_group_id,
                     uint16  des_app_id,
                     uint8   range,
                     int8    msg_priority,
                     uint8   msg_type,
                     uint32  msg_id,
                     message_t* msg);

//error_no_t send_msg(message_t* msg);
void fill_msg_body(void* msg_content, uint32 msg_len, message_t* msg);

void free_msg_buff(message_t** msg);

void print_msg_header(message_t* msg);

void print_msg(message_t* msg);

message_t* clone_message(message_t* msg);

#ifdef __cplusplus
}
#endif

#endif
