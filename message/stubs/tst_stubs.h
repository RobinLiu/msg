#ifndef TST_TEST_STUB_H
#define TST_TEST_STUB_H
#ifdef __cplusplus
extern "C" {
#endif
#include "basic_types.h"

/*void set_mac(uint8* mac, void* buf);

void fill_header(uint8* buf, int8 pdu_type, uint16 frag_len, uint16 msg_seq);
void fill_content(uint8* buf, uint32 content_len);

void* generate_package(uint8 pdu_type, uint32 content_len, uint16 seq);*/

void* gen_ack(uint16 seq);

void* gen_retrans_req(uint16 seq);

void* gen_data(uint16 seq, uint32 data_len);

message_t* gen_msg(uint32 msg_len);

#ifdef __cplusplus
}
#endif

#endif
