#ifndef MESSAGE_SYSTEM_BASIC_TYPES_H
#define MESSAGE_SYSTEM_BASIC_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif
#include "build.h"

typedef signed char         int8;
typedef short               int16;
typedef int                 int32;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;

#ifndef __cplusplus
typedef uint8               bool;
#define TRUE                (bool)1;
#define FALSE               (bool)0;
#endif

typedef int32               error_no_t;

#define RANGE_STANDBY       1
#define RANGE_ACTIVE        2
#define RANGE_BOTH          3

#define   BUFF_MAGIC_HEADER     0xbeafdead
#define   BUFF_MAGIC_TAIL       0xdaedfaeb

#define MSG_TYPE_SYNC_REQ       1
#define MSG_TYPE_SYNC_RSP       2
#define MSG_TYPE_ASYNC_MSG      3

typedef struct {
  uint8   group_id;
  uint16  app_id;
  uint8   role;
} msg_receiver_t;

typedef  msg_receiver_t     msg_sender_t;

typedef uint16               node_id_t;

typedef struct list_head {
   struct list_head *next, *prev;
} list_head_t;

typedef struct  {
#if DBG_MSG
  uint32         magic_num;
#endif
  msg_receiver_t rcver;
  msg_sender_t   snder;
  uint8          msg_type;
  int8           priority;
  uint32         msg_len;
  uint32         msg_seq;
  uint32         ack_seq;
  uint32         msg_id;
} msg_header_t;

typedef struct {
  uint32        magic_num;
} msg_tail_t;

typedef struct {
  msg_header_t*  header;
  uint8*         body;
#if DBG_MSG
  msg_tail_t*    tail;
#endif
  uint8*         buf_head;
  uint32         buf_len;
  uint8*         buf_ptr;
  struct list_head list;
} message_t;

#define MAC_ADDR_LEN 6
#define MAC_ADDR_STR_LEN  18
#define ETH_P_MSG 0x88aa

#define MSG_ID_NODE_STATUS  1

typedef struct node_status {
  uint8 node_id;
  uint8 node_status;
} node_status_t;


#ifdef __cplusplus
}
#endif
#endif
