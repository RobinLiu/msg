#ifndef MESSAGE_LINK_H
#define MESSAGE_LINK_H
#ifdef __cplusplus
extern "C" {
#endif
#include "common/include/common.h"
#include "common/include/common_list.h"

//using layer2 to transport the message.
#if USING_LAYER2

#define PROTO_VER     0

#define PDU_RESET      0
#define PDU_RESET_ACK  1
#define PDU_HELLO      2
#define PDU_HELLO_ACK  3
#define PDU_DATA       4
#define PDU_ACK        5
#define PDU_RETRANS    6


typedef struct link_header {
  uint8  version;
  uint8  pdu_type;
  node_id_t dst_node;

  node_id_t src_node;
  uint16 frag_len;

  uint16 frag_seq;
  uint16 frag_index;

  uint32 msg_seq;

  uint8  is_last_frag;
  uint8  _pad[3];
} link_header_t;

#define ETH_HEADER_LEN    14

#if ENABLE_VLAN
#define VLAN_HEADER_LEN   2
#else
#define VLAN_HEADER_LEN   0
#endif

#define LINK_HEADER_LEN   sizeof(link_header_t)
#define TOTAL_HEADER_LEN  (ETH_HEADER_LEN + VLAN_HEADER_LEN + LINK_HEADER_LEN)
//#define LINK_PAYLOAD_LEN  1472
#define LINK_PAYLOAD_LEN  65535
#define MAX_FRAME_PAYLOAD_LEN     (LINK_PAYLOAD_LEN - TOTAL_HEADER_LEN)

#define MAX_RETRNS_COUNT 8
#else //Using layer3 to transport the message (UDP)
typedef struct {

} msg_link_t;
#endif

void init_msg_link();

error_no_t send_msg_to_node(message_t* msg, node_id_t dest_node);

void on_pkt_received(void* pkt, uint32 pkt_len);

#ifdef __cplusplus
}
#endif
#endif
