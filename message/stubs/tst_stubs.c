#include "tst_stubs.h"
#include "basic_types.h"
#include "msg_link.h"
#include <string.h>

void set_mac(uint8* mac, void* buf) {
  uint8* ptr = (uint8*)(buf);
  memcpy(ptr, mac, 6);
}

void fill_link_header(uint8* buf, int8 pdu_type, uint16 frag_len, uint16 msg_seq) {
  void* buf_ptr = buf;
  uint8 dest_mac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  uint8 src_mac[6] = {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa};

  set_mac(dest_mac, buf_ptr);
  buf_ptr += 6;

  set_mac(src_mac, buf_ptr);
  buf_ptr += 6;

  *((uint16*)buf_ptr) = ETH_P_MSG;
  buf_ptr += 2;

  static uint16 frag_seq = 0;
  link_header_t* lh = (link_header_t*)buf_ptr;
  lh->frag_index = 0;
  lh->frag_seq = frag_seq;
  lh->is_last_frag = 1;
  lh->msg_seq = msg_seq;
  lh->frag_len = frag_len;
  lh->pdu_type = pdu_type;
  lh->src_node = 1;
  lh->dst_node = 0;
}

void fill_content(uint8* buf, uint32 content_len) {
  void* buf_ptr = buf;
  buf_ptr += (TOTAL_HEADER_LEN);
  msg_header_t* mh = (msg_header_t*)buf_ptr;
  mh->msg_len=content_len - sizeof(msg_header_t) -sizeof(msg_tail_t);
  uint32 i = 0;

  buf_ptr += sizeof(msg_header_t);
  uint8* msg = (uint8*)buf_ptr;
  for(; i < mh->msg_len; ++i) {
    *msg++ = 'a' + (i%26);
  }

  uint32* tail = (uint32*)(buf_ptr + mh->msg_len);

  *tail = BUFF_MAGIC_TAIL;
}

void* generate_package(uint8 pdu_type, uint32 content_len, uint16 seq) {
  uint32 data_len = TOTAL_HEADER_LEN + content_len;
  void* buf = malloc(data_len);
  memset(buf, 0, data_len);

  fill_link_header((uint8*)buf, pdu_type, content_len, seq);

  if(pdu_type == PDU_DATA) {
    fill_content(buf, content_len);
  }

  return buf;
}


void* gen_ack(uint16 seq){
  return generate_package(PDU_ACK, 0, seq);
}


void* gen_retrans_req(uint16 seq) {
  return generate_package(PDU_RETRANS, 0, seq);
}


void* gen_data(uint16 seq, uint32 data_len) {
  void* data = generate_package(PDU_DATA, data_len, seq);
//  dbg_print_msg(data);
  return data;
}

//message_t* gen_msg(uint32 msg_len) {
//  message_t* msg = NULL;
//
//  msg = allocate_msg_buff(msg_len);
//  uint8 group_id = 16;
//  uint16 app_id = 32;
////  fill_msg_header(group_id, app_id, RANGE_ACTIVE, 4, msg);
//  uint32 i = 0;
//  uint8* buf = msg->body;
//  for(i = 0; i < msg_len; ++i) {
//    *buf++ = 'a' + (i%26);
//  }
//  msg->header->msg_len = msg_len;
//  return msg;
//}
