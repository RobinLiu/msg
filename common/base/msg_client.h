#ifndef BASE_MSG_CLIENT_H
#define BASE_MSG_CLIENT_H
#include "common/include/basic_types.h"

void init_msg_system();

error_no_t send_msg(uint8  dst_grp_id,
                    uint16 dst_app_id,
                    uint32 msg_id,
                    uint8  msg_priority,
                    void*  msg_content,
                    uint32 msg_len);

void receive_msg(message_t** msg);

error_no_t send_sync_msg(uint8  dst_grp_id,
                    uint16 dst_app_id,
                    uint32 msg_id,
                    uint8  msg_priority,
                    void*  msg_content,
                    uint32 msg_len,
                    uint32 time_out,
                    void*  rsp_buf,
                    uint32 rsp_buf_size);

error_no_t send_sync_msg_rsp(message_t* syn_req_msg,
                    void* rsp_buf,
                    int rsp_buf_size);

#endif
