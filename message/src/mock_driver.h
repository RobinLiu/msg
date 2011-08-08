#ifndef MOCK_DRIVER_H
#define MOCK_DRIVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "common/include/basic_types.h"

void driver_xmit_pkt(int connfd, void* conndata, void* buff, uint32 buff_len);

void create_rcv_thread();

void dbg_print_msg(uint8* msg);

#ifdef __cplusplus
}
#endif

#endif
