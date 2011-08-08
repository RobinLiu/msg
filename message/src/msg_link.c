#include "msg_link.h"
#include "mock_driver.h"
#include "common/base/thread.h"
#include "common/base/timer.h"
#include "common/base/lock.h"
#include "common/base/message.h"
#include "message/mock_API/app_info.h"
#include "message_router.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
/******************************************************************************
 * MACRO definitions
 *****************************************************************************/
/* Timer values. Unit is ms */
#define RETRANS_TIMEOUT 1000

// the ring buffer size, MUST be 2^n
#define TX_WINDOW_MAX_SIZE (0x1 << 5)

#define WIN_INDEX_ADD(index, offset) ((index + offset) & (TX_WINDOW_MAX_SIZE - 1))
#define WIN_INDEX_SUB(index1, index2) ((index1 + TX_WINDOW_MAX_SIZE - index2) & (TX_WINDOW_MAX_SIZE - 1))

#define FRAME_SEQ_BITS 16
#define FRAME_SEQ_MAX ((0x1 << FRAME_SEQ_BITS) - 1)
/* seq1 subtract seq2 */
#define FRAME_SEQ_SUB(seq1, seq2) ((seq1 + FRAME_SEQ_MAX + 1 - seq2) & FRAME_SEQ_MAX)
/* seq1 greater than seq2 */
#define FRAME_SEQ_GT(seq1, seq2) (FRAME_SEQ_SUB(seq1, seq2) <= (FRAME_SEQ_MAX >> 1))

/******************************************************************************
 * type definitions
 *****************************************************************************/
//TODO: initialization the list;
typedef struct msg_frag {
  uint8* data;
  uint8* ptr_ref;
  uint16 data_len;
  uint16 _reserved;
  struct list_head list;
} msg_frag_t;

typedef struct {
   struct msg_frag* frag;
} tx_win_item_t;

#define LINK_NOT_INITIALIZED  0
#define LINK_STATUS_INIT      1
#define LINK_STATUS_OK        2

#if USING_UDP_FOR_MESSAGE
typedef struct conn_info {
  int connfd;
  struct sockaddr_in sockinfo;
}conn_info_t;
#endif

typedef struct link {

  node_id_t         peer;
  msg_timer_t       retrans_timer;
  uint8             status;
  uint8             peer_status;
  lock_t            lock;
#if USING_UDP_FOR_MESSAGE
  conn_info_t       conn_info;
#endif
  struct {
    uint16          frame_seq;
    uint16          retrans_timeout_count;
    uint16          win_start;
    uint16          win_end;
    tx_win_item_t   win[TX_WINDOW_MAX_SIZE];
    lock_t          tx_lock;
    thread_cond_t   tx_queue_threshold;
    thread_id_t     tx_thread_id;
    thread_id_t     tx_mutex_holder;

    struct          list_head msg_queue;
//    lock_t          msg_queue_lock;
    //statistics
    uint32          msg_queue_size;
    uint32          num_of_sent_msg;
    uint32          num_of_sent_frag;
    uint32          num_of_acked_frag;
    uint32          num_of_ack_sent;
  } tx;

  struct {
    uint16          exp_frame_seq;
    message_t*      message;
    lock_t          rx_lock;
    //statistics
    uint32          num_of_rcvd_frag;
    uint32          num_of_rcvd_msg;
  } rx;
} msg_link_t;

/******************************************************************************
 * Global variables
 *****************************************************************************/
msg_link_t* g_msg_link[MAX_NODE_NUM];
static uint16 g_tx_window_size = TX_WINDOW_MAX_SIZE - 1;
lock_t g_link_table_lock;

/******************************************************************************
 * Functions
 *****************************************************************************/
msg_link_t* get_msg_link(node_id_t node_id);

void init_msg_link() {
  uint32 i = 0;

  init_lock(&g_link_table_lock);

  lock(&g_link_table_lock);
  for(; i < MAX_NODE_NUM; ++i ) {
    g_msg_link[i] = NULL;
  }
  unlock(&g_link_table_lock);

}

msg_frag_t* alloc_msg_frag(uint32 payload_len) {

  msg_frag_t* msg_frag = (msg_frag_t*)malloc(sizeof(msg_frag_t));
  CHECK(NULL != msg_frag);

  msg_frag->ptr_ref = (uint8*) malloc(payload_len + TOTAL_HEADER_LEN);
  CHECK(NULL != msg_frag->ptr_ref);

  msg_frag->data = msg_frag->ptr_ref + TOTAL_HEADER_LEN;
  msg_frag->data_len = payload_len;

  return msg_frag;
}

void free_msg_frag(msg_frag_t** msg_frag) {
  if(NULL == msg_frag || (*msg_frag) == NULL) {
    return;
  }

  if(NULL != (*msg_frag)->ptr_ref) {
    free((*msg_frag)->ptr_ref);
    (*msg_frag)->ptr_ref = NULL;
  }

  free(*msg_frag);
  (*msg_frag) = NULL;
}

static void fill_msg_to_msg_frag(message_t* msg, msg_frag_t* msg_frag) {
  if (msg->buf_len > MAX_FRAME_PAYLOAD_LEN) {
    msg_frag->data_len = MAX_FRAME_PAYLOAD_LEN;
  } else {
    msg_frag->data_len = msg->buf_len;
  }

  memcpy((void*)msg_frag->data, (void*)msg->buf_ptr, msg_frag->data_len);
  msg->buf_ptr += msg_frag->data_len;
  msg->buf_len -= msg_frag->data_len;
}


static bool sliding_window_is_empty(msg_link_t* link)
{
   return (uint8)(link->tx.win_end == link->tx.win_start);
}


static bool sliding_window_is_full(msg_link_t* link)
{
   return (WIN_INDEX_SUB(link->tx.win_end, link->tx.win_start) >= g_tx_window_size);
}


static uint16 sliding_window_get_used_size(msg_link_t* link) {
  return WIN_INDEX_SUB(link->tx.win_end, link->tx.win_start);
}


static int32 sliding_window_get_ind(uint16 frame_seq, msg_link_t* link) {
  uint16 in_flight_frames, tail_frames;
  tail_frames = FRAME_SEQ_SUB(link->tx.frame_seq, frame_seq);
  in_flight_frames = sliding_window_get_used_size(link);
  if (tail_frames > in_flight_frames || tail_frames == 0) {
    LOG(WARNING, "sliding_window_get_ind failed");
    return -1;
  }
  return (int32)WIN_INDEX_SUB(link->tx.win_end, tail_frames);
}


static void sliding_window_move_to(uint16 new_win_start, msg_link_t* link) {
  uint16 ind;
  for(ind = link->tx.win_start; ind != new_win_start; ind = WIN_INDEX_ADD(ind, 1)) {
    free_msg_frag(&(link->tx.win[ind].frag));
    link->tx.win[ind].frag = NULL;
  }
  link->tx.win_start = new_win_start;
}


static uint32 get_msg_link_idx(node_id_t node_id) {
  //TODO: map node id to link index;
  return node_id;
}

static void prepare_pkt(msg_frag_t* msg_frag, node_id_t peer) {
  //TODO: fill layer2 header here;
  uint8 dest_mac[MAC_ADDR_LEN];
  uint8 self_mac[MAC_ADDR_LEN];
  get_self_mac(self_mac);
  get_peer_mac(peer, dest_mac);

  msg_frag->data -= ETH_HEADER_LEN;
  msg_frag->data_len += ETH_HEADER_LEN;
  uint8* data_ptr = msg_frag->data;

  /*ethernet header contains following field:
   * |dest mac  | src mac|optinal vlan tag| ether type|
   * |6(octets) |6       |4               | 2         |
   * */
  memcpy(data_ptr, dest_mac, MAC_ADDR_LEN);
  memcpy(data_ptr + MAC_ADDR_LEN, self_mac, MAC_ADDR_LEN);
  data_ptr += MAC_ADDR_LEN * 2;
  *((uint16*)data_ptr) = ETH_P_MSG; //TODO: htons maybe needed.
}


static void conn_xmit_pkt(msg_frag_t* msg_frag, node_id_t peer) {
  msg_link_t* link = get_msg_link(peer);
  prepare_pkt(msg_frag, peer);
  driver_xmit_pkt(link->conn_info.connfd,
                  (void*)&link->conn_info.sockinfo,
                  (void*)msg_frag->data,
                  msg_frag->data_len);
  LOG(INFO, "xmit frag: %d", link->tx.frame_seq);
}


static void tx_msg_enqueue(message_t* msg, msg_link_t* link) {
  lock(&link->tx.tx_lock);
  bool was_empty = list_empty(&link->tx.msg_queue);
  //TODO: limit the number of message in the queue to avoid too many messages.
  list_add_tail(&msg->list, &link->tx.msg_queue);
  link->tx.msg_queue_size++;
  if (was_empty) {
    LOG(INFO, "new message arrived, notify thread...");
    notify_thread(&link->tx.tx_queue_threshold);
  }
  unlock(&link->tx.tx_lock);
}

static void send_zero_msg(uint16 frame_seq, uint8 msg_type, node_id_t peer, uint32 msg_seq) {
  msg_frag_t* msg = alloc_msg_frag(0);
  CHECK(NULL != msg);
  link_header_t* lh = (link_header_t*)(msg->data - sizeof(link_header_t));
  lh->version = PROTO_VER;
  lh->pdu_type = msg_type;
  lh->src_node = get_self_node_id();
  lh->dst_node = peer;
  lh->frag_seq = frame_seq;
  lh->frag_len = 0;
  lh->frag_index = 0;
  lh->msg_seq   = msg_seq;
  lh->is_last_frag = 1;
  LOG(INFO, "frag_seq is %d", lh->frag_seq);
  msg->data_len += sizeof(link_header_t);
  msg->data -= sizeof(link_header_t);
  conn_xmit_pkt(msg, peer);
  free_msg_frag(&msg);
}

#if ENABLE_RESET_LINK
static void send_reset_req(msg_link_t* link) {
  send_zero_msg(link->rx.exp_frame_seq, PDU_RESET, link->peer, 0);
}

static void send_reset_ack(msg_link_t* link) {
  send_zero_msg(link->rx.exp_frame_seq, PDU_RESET_ACK, link->peer, 0);
}
#endif

static void send_hello(msg_link_t* link) {
  if (link->status != LINK_STATUS_OK) {
    LOG(INFO, "Begin to send hello");
    send_zero_msg(link->tx.frame_seq, PDU_HELLO, link->peer, link->status);
    LOG(INFO, "end send hello");
  }
}


static void send_hello_ack(msg_link_t* link) {
  LOG(INFO, "Begin to send hello ack");
  send_zero_msg(link->tx.frame_seq, PDU_HELLO_ACK, link->peer, link->status);
  LOG(INFO, "end send hello ack");
}


static void send_ack(link_header_t* lh) {
  LOG(INFO,"ack for seq %d", lh->frag_seq);
  send_zero_msg(lh->frag_seq, PDU_ACK, lh->src_node, lh->msg_seq);
}


static void send_retrans_req(msg_link_t* link) {
  send_zero_msg(link->rx.exp_frame_seq, PDU_RETRANS, link->peer, 0);
}


static void send_retrans(uint16 win_ind, msg_link_t* link) {
  uint16 ind;
  if(win_ind > g_tx_window_size){
    LOG(ERROR, "Invalid windows index: %d", win_ind);
    return;
  }
  ind = win_ind;
  for(ind = win_ind; ind != link->tx.win_end; ind = WIN_INDEX_ADD(ind, 1)) {
    //make sure that the frame buffer has not been freed.
    LOG(ERROR, "Retrans wind[%d]", ind);
    CHECK(NULL != link->tx.win[ind].frag);
//    conn_xmit_pkt(link->tx.win[ind].frag, link->peer);
    msg_frag_t* msg_frag = link->tx.win[ind].frag;
    driver_xmit_pkt(link->conn_info.connfd,
        (void*)&link->conn_info.sockinfo,(void*)msg_frag->data, msg_frag->data_len);

  }
}


static error_no_t put_msg_into_sliding_window(message_t* msg, msg_link_t* link) {
  if(NULL == msg || NULL == link) {
    LOG(WARNING, "Pass NUll pointer to function");
    return NULL_POINTER_EC;
  }

  bool was_empty = sliding_window_is_empty(link);
  uint16 frag_index = 0;

  while (msg->buf_len > 0) {
//    LOG(INFO, "msg->buf_len: %d", msg->buf_len);
    if(sliding_window_is_full(link)) {
      //should not enter here, since we have already checked with lock before.
      LOG(WARNING, "sliding window is full. Wait to be notified...");
      wait_to_be_notified(&link->tx.tx_queue_threshold, &link->tx.tx_lock);
    }
    bool is_last_frag = (msg->buf_len <= MAX_FRAME_PAYLOAD_LEN);
    uint32 payload_len = msg->buf_len > MAX_FRAME_PAYLOAD_LEN
        ? MAX_FRAME_PAYLOAD_LEN : msg->buf_len;

    msg_frag_t* msg_frag = alloc_msg_frag(payload_len);
    CHECK(NULL != msg_frag);

    //copy content to msg frag;
    fill_msg_to_msg_frag(msg, msg_frag);

    //get link header ptr
    msg_frag->data -= sizeof(link_header_t);
    msg_frag->data_len += sizeof(link_header_t);
    link_header_t* lh = (link_header_t*)msg_frag->data;

    //fill link header
    lh->version = PROTO_VER;
    lh->pdu_type = PDU_DATA;
    lh->dst_node = link->peer;
    lh->src_node = get_self_node_id();
    lh->frag_len = payload_len;
    lh->frag_seq = link->tx.frame_seq++;
    lh->frag_index = frag_index++;
    lh->msg_seq = msg->header->msg_seq;
    lh->is_last_frag = is_last_frag;

    uint16 end = link->tx.win_end;
    link->tx.win[end].frag = msg_frag;
    LOG(INFO, "trans wind[%d]", end);
    //move sliding window to next;
    link->tx.win_end = WIN_INDEX_ADD(end, 1);
    //send to driver;
    conn_xmit_pkt(link->tx.win[end].frag, link->peer);
    link->tx.num_of_sent_frag++;
  }

  if(was_empty && !sliding_window_is_empty(link)) {
    renew_timer(&link->retrans_timer, RETRANS_TIMEOUT);
  }

  return SUCCESS_EC;
}


static void* tx_thread_worker(void* arg){
  CHECK(NULL != arg);
  msg_link_t* link = ( msg_link_t*)arg;

  message_t* msg = NULL;
  send_hello(link);

  while(1) {
    lock(&link->tx.tx_lock);
//    LOG(INFO, "TX thread while...");
    if(list_empty(&link->tx.msg_queue) ||
        sliding_window_is_full(link) ||
        link->status != LINK_STATUS_OK ||
        link->peer_status != LINK_STATUS_OK) {
//      LOG(INFO, "Sleep...");
      wait_to_be_notified(&link->tx.tx_queue_threshold, &link->tx.tx_lock);
//      LOG(INFO, "TX thread waked up, check the conditions....");
    }

//    lock(&link->lock);
    if(link->status != LINK_STATUS_OK || link->peer_status != LINK_STATUS_OK) {
      LOG(WARNING, "Link stats is not ok");
      unlock(&link->tx.tx_lock);
      send_hello(link);
      //TODO: how to deal with if link is not ok?
      continue;
    }
//    unlock(&link->lock);
    if(list_empty(&link->tx.msg_queue)) {
      unlock(&link->tx.tx_lock);
      continue;
    }

    if(sliding_window_is_full(link)) {
      LOG(WARNING,"sliding window is full");
      unlock(&link->tx.tx_lock);
      continue;
    }

//    LOG(INFO, "Begin to put message to sliding window");
    //get one message from message queue and put it into sliding window;
    msg = list_entry(link->tx.msg_queue.next, message_t, list);
    CHECK(NULL != msg);
    put_msg_into_sliding_window(msg, link);
//    LOG(INFO, "finished send one package");

    //this message has been put to sliding window, delete it
    list_del(&msg->list);
    free_msg_buff(&msg);
    link->tx.msg_queue_size--;
    link->tx.num_of_sent_msg++;
    LOG(INFO, "Num of msg send is: %d", link->tx.num_of_sent_msg);

    unlock(&link->tx.tx_lock);
  }

  return (void*)(0);
}

void timeout_handle(void* data) {
  CHECK(NULL != data);
  msg_link_t* link = (msg_link_t*)data;
  if (link->retrans_timer.state == 1) {
    return;
  }
  LOG(INFO, "Timeout for msg ack, retrans it.");
  lock(&link->tx.tx_lock);
  LOG(INFO, "Locked");
  if (link->tx.retrans_timeout_count < MAX_RETRNS_COUNT) {
    send_retrans(link->tx.win_start, link);
    link->tx.retrans_timeout_count++;
  } else {
    LOG(WARNING, "Max retry count reached, drop the fragment.");
    stop_timer(&link->retrans_timer);
    //drop the frag and move to next frag.
    uint16 new_win_start = WIN_INDEX_ADD(link->tx.win_start, 1);
    //del msg_frag from sliding window
    sliding_window_move_to(new_win_start, link);
    link->tx.retrans_timeout_count = 0;
  }
  //TODO: find a better solution for timer,
  bool is_empty = sliding_window_is_empty(link);
  unlock(&link->tx.tx_lock);
  if (!is_empty) {
    renew_timer(&link->retrans_timer, RETRANS_TIMEOUT);
  }
  LOG(INFO, "Out timeout_handle");
}

static void get_conn_info(node_id_t node_id, conn_info_t* conn_info) {

  int8 serv_ip_addr[32] = {0};
  uint16 serv_port = 0;
  get_serv_info(node_id, serv_ip_addr, &serv_port);
  memset(&conn_info->sockinfo, 0, sizeof(conn_info->sockinfo));
  conn_info->sockinfo.sin_family = AF_INET;
  conn_info->sockinfo.sin_port = htons(serv_port);
  inet_pton(AF_INET, (char*)serv_ip_addr, (void*)&conn_info->sockinfo.sin_addr);
  //inet_aton

  conn_info->connfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  CHECK(-1 != conn_info->connfd);
}


static void init_link(node_id_t node_id, msg_link_t* link) {
  INIT_LIST_HEAD(&link->tx.msg_queue);
  init_lock(&link->lock);
  init_lock(&link->tx.tx_lock);
  init_lock(&link->rx.rx_lock);
//  init_lock(&link->rx.msg_queue_lock);
  link->peer = node_id;
  get_conn_info(node_id, &link->conn_info);
  // create_udp_client();
  thread_cond_init(&link->tx.tx_queue_threshold);
  link->status = LINK_NOT_INITIALIZED;
  //create thread to send package

  uint32 timeout = RETRANS_TIMEOUT;
  setup_timer(&link->retrans_timer, &timeout_handle, (void*)link, timeout);
  create_thread(&link->tx.tx_thread_id,
               NULL,
               tx_thread_worker,
               (void*)link);
}


msg_link_t* get_msg_link(node_id_t node_id) {
  uint32 link_idx = get_msg_link_idx(node_id);
//  LOG(INFO, "link_idx is %d", link_idx);
  if(link_idx >= MAX_NODE_NUM) {
    LOG(WARNING,"Get msg link faild");
    return NULL;
  }
  lock(&g_link_table_lock);
  if(g_msg_link[link_idx] != NULL) {
    if (g_msg_link[link_idx]->conn_info.connfd == -1) {
      get_conn_info(g_msg_link[link_idx]->peer, &g_msg_link[link_idx]->conn_info);
    }
    unlock(&g_link_table_lock);
    return g_msg_link[link_idx];
  }

  //new connection
  msg_link_t* new_link = (msg_link_t*)malloc(sizeof(msg_link_t));
  memset(new_link, 0, sizeof(msg_link_t));
  CHECK(NULL != new_link);

  //TODO: initialization for the new link.
  init_link(node_id, new_link);

  g_msg_link[link_idx] = new_link;
  unlock(&g_link_table_lock);
  return g_msg_link[link_idx];
}


error_no_t send_msg_to_node(message_t* msg, node_id_t dest_node) {
  msg_link_t* msg_link = NULL;
  msg_link = get_msg_link(dest_node);
  if(msg_link == NULL) {
    LOG(WARNING, "Get msg link failed");
    return GET_MSG_LINK_FAILED_EC;
  }
  tx_msg_enqueue(msg, msg_link);

  return SUCCESS_EC;
}

#if ENABLE_RESET_LINK
static void handle_reset(uint8* pkt, msg_link_t* link) {
  link_header_t* lh = (link_header_t*)(pkt + ETH_HEADER_LEN);

  link->status = LINK_STATUS_INIT;

  lock(&link->tx.tx_lock);
  //reset all the fragement on the go;
  int32 i = 0;
  for(i = 0; i < g_tx_window_size; ++i) {
    if (link->tx.win[i].frag != NULL ) {
      free_msg_frag(&(link->tx.win[i].frag));
      link->tx.win[i].frag = NULL;
    }
  }
  link->tx.win_start = 0;
  link->tx.win_end = 0;
  link->tx.frame_seq = 0;

  //deal with half sent message;
  //TODO: how about the enqueued message which has not been sent out.
  if (!list_empty(&link->tx.msg_queue)) {
    message_t* msg = list_entry(link->tx.msg_queue.next, message_t, list);
    CHECK(NULL != msg);
    if (msg->buf_head != msg->buf_ptr) {
      //already transfered part of the message, remove it.
      list_del(&msg->list);
      free_msg_buff(&msg);
      link->tx.msg_queue_size--;
    }
  }

  unlock(&link->tx.tx_lock);

  lock(&link->rx.rx_lock);
  link->rx.exp_frame_seq = lh->frag_seq;
  send_reset_ack(link);
//  link->status = LINK_STATUS_OK;
  unlock(&link->rx.rx_lock);
}

static void handle_reset_ack(uint8* pkt, msg_link_t* link) {
  //send_hello(link);
}
#endif


static void handle_hello(uint8* pkt, msg_link_t* link) {

  lock(&link->lock);
  lock(&link->rx.rx_lock);
  LOG(INFO, "Hello is received");
  link_header_t* lh = (link_header_t*)pkt;
  link->rx.exp_frame_seq = lh->frag_seq;
  link->peer_status = LINK_STATUS_OK;
  send_hello_ack(link);
  link->status = LINK_STATUS_OK;
  unlock(&link->rx.rx_lock);
  unlock(&link->lock);
}


static void handle_hello_ack(uint8* pkt, msg_link_t* link) {
  lock(&link->lock);
  lock(&link->rx.rx_lock);
  LOG(INFO, "Hello ack is received");
  link_header_t* lh = (link_header_t*)pkt;
  link->peer_status = lh->msg_seq;
  link->rx.exp_frame_seq = lh->frag_seq;
  unlock(&link->rx.rx_lock);
  link->status = LINK_STATUS_OK;
  link->peer_status = LINK_STATUS_OK;

  if (LINK_STATUS_OK == link->peer_status) {
    notify_thread(&link->tx.tx_queue_threshold);
  } else {
    LOG(INFO, "peer is not ok, send hello again");
    send_hello(link);
  }
  unlock(&link->lock);
}

static void handle_data(uint8* pkt, msg_link_t* link) {
  link_header_t* lh = (link_header_t*)pkt;
  lock(&link->rx.rx_lock);
  if(lh->frag_seq == link->rx.exp_frame_seq) {
    LOG(INFO, "Correct seq received, seq is %d",lh->frag_seq );
    //send ack to peer
    link->rx.exp_frame_seq++;
    link->rx.num_of_rcvd_frag++;
    send_ack(lh);
    link->tx.num_of_ack_sent++;
    if(lh->frag_index == 0) {
      //first frame of a message, read header and alloc message buf;
//      LOG(INFO, "lh->frag_len %d sizeof(msg_header_t) %d.", lh->frag_len, sizeof(msg_header_t));
      CHECK(lh->frag_len >= sizeof(msg_header_t));
      msg_header_t* msg_header = (msg_header_t*)(pkt + sizeof(link_header_t));
      uint32 msg_len = msg_header->msg_len;

      message_t* new_msg = allocate_msg_buff(msg_len);
      CHECK(NULL != new_msg);
      if(NULL != link->rx.message) {
        //TODO: maybe should move the free to router, to make sure that the
        //message has been delivered to user's program.
        free_msg_buff(&link->rx.message);
        link->rx.message = NULL;
      }
      //set the buf that has been copied as 0;
      check_buff_magic_num(new_msg);
      new_msg->buf_len = 0;
      link->rx.message = new_msg;
    } else if (link->rx.message == NULL ||
        link->rx.message->header->msg_seq != lh->msg_seq) {
      LOG(WARNING, "First frame of a message is not received, drop the fragment");
      unlock(&link->rx.rx_lock);
      //CHECK(0 == 1);
      return;
    }
    uint8* msg_data = (uint8*)(pkt + sizeof(link_header_t));
    memcpy(link->rx.message->buf_ptr, msg_data, lh->frag_len);

    link->rx.message->buf_ptr += lh->frag_len;
    link->rx.message->buf_len += lh->frag_len;

    if(lh->is_last_frag) {
      //this is the last of the frag;
      check_buff_magic_num(link->rx.message);
      uint32 total_len = sizeof(msg_header_t) + link->rx.message->header->msg_len + sizeof(msg_tail_t);
//      LOG(INFO,"total_len %d, received len %d", total_len, link->rx.message->buf_len);
      CHECK(link->rx.message->buf_len == total_len);
      //send message to router
      router_receive_msg(link->rx.message);
      free_msg_buff(&link->rx.message);
      link->rx.num_of_rcvd_msg++;
      LOG(ERROR, "Num of msg received is: %d", link->rx.num_of_rcvd_msg);
//      if (NULL != link->rx.message) {
//        free_msg_buff(&link->rx.message);
//      }
    }
  } else if (FRAME_SEQ_GT(lh->frag_seq , link->rx.exp_frame_seq)){
    //CHECK(0 == 1);
    //unexpectd frame received, ignore or request retrans
    LOG(WARNING, "Retrans reques for frame seq: %d", link->rx.exp_frame_seq);
    send_retrans_req(link);

  } else { //duplicated buff received, send ack to peer and drop the packet
    //CHECK(0 == 1);
    LOG(WARNING, "Duplicated package received ,send ack and drop it");
    send_ack(lh);
    link->tx.num_of_ack_sent++;
  }
  unlock(&link->rx.rx_lock);
}


static void handle_ack(void* pkt, msg_link_t* link) {
//  LOG(INFO, "ack is received.");
  link_header_t* lh = (link_header_t*)pkt;

  lock(&link->tx.tx_lock);
  bool was_full = sliding_window_is_full(link);
  int32 win_ind_ret = sliding_window_get_ind(lh->frag_seq, link);
  if(win_ind_ret < 0 ) {
    LOG(WARNING, "Incorrect ack received, drop it");
    send_hello(link);
    unlock(&link->tx.tx_lock);
    return;
  }


  LOG(INFO, "ack is received for frag:%d", lh->frag_seq);
  uint16 win_ind = (uint16)win_ind_ret;
  uint16 new_win_start = WIN_INDEX_ADD(win_ind, 1);
  //del msg_frag from sliding window
  sliding_window_move_to(new_win_start, link);
  link->tx.retrans_timeout_count = 0;
  link->tx.num_of_acked_frag++;

  LOG(ERROR,"num_of_acked_frag %d", link->tx.num_of_acked_frag);
//  if(link->tx.num_of_acked_frag == 10000) {
//    exit(0);
//  }
  //Notify sender to continuE send if sliding window was full;
  if(was_full) {
    notify_thread(&link->tx.tx_queue_threshold);
  }
  bool is_empty = sliding_window_is_empty(link);

  unlock(&link->tx.tx_lock);
  if (!is_empty) {
    renew_timer(&link->retrans_timer, RETRANS_TIMEOUT);
//    start_timer(&link->retrans_timer);
  } else {
    stop_timer(&link->retrans_timer);
  }
}


static void handle_retrans_req(void* pkt, msg_link_t* link) {
  //CHECK(0 == 1);
  lock(&link->tx.tx_lock);
  link_header_t* lh = (link_header_t*)pkt;
  int32 win_ind_ret = sliding_window_get_ind(lh->frag_seq, link);
  if(win_ind_ret < 0 ) {
    LOG(WARNING, "Incorrect retrans req received, drop it");
    unlock(&link->tx.tx_lock);
    send_hello(link);
    return;
  }
  uint16 win_ind = (uint16)win_ind_ret;

  send_retrans(win_ind, link);
  unlock(&link->tx.tx_lock);
}


static bool is_pkt_valid(void* pkt, uint32 pkt_len) {
  if(pkt == NULL || pkt_len < TOTAL_HEADER_LEN) {
    return FALSE;
  }
  //TODO: other check;

  return TRUE;
}

uint8* strip_eth_header(void* pkt) {
  uint8* frag_ment = (uint8*)((uint8*)pkt + ETH_HEADER_LEN);
  return frag_ment;
}

void on_pkt_received(void* pkt, uint32 pkt_len) {
  if(!is_pkt_valid(pkt, pkt_len)) {
    LOG(WARNING, "Package received, but failed to pass the check, drop it.");
    return;
  }

  uint8* frag_ment = strip_eth_header(pkt);
  link_header_t* lh = (link_header_t*)(frag_ment);
  msg_link_t* link = get_msg_link(lh->src_node);

  if(NULL == link) {
    LOG(WARNING, "Get link for message failed, node id is %d, drop the message.",
        lh->src_node);
    return;
  }

  switch(lh->pdu_type) {
#if  ENABLE_RESET_LINK
    case PDU_RESET:
      handle_reset(frag_ment, link);
      break;
    case PDU_RESET_ACK:
      handle_reset_ack(frag_ment, link);
      break;
#endif
    case PDU_HELLO:
      handle_hello(frag_ment, link);
      break;
    case PDU_HELLO_ACK:
      handle_hello_ack(frag_ment, link);
      break;
    case PDU_DATA:
      if (link->status == LINK_STATUS_OK) {
        handle_data(frag_ment, link);
      }
      break;
    case PDU_ACK:
      if (link->status == LINK_STATUS_OK) {
        handle_ack(frag_ment, link);
      }
      break;
    case PDU_RETRANS:
      if (link->status == LINK_STATUS_OK) {
        handle_retrans_req(frag_ment, link);
      }
      break;
    default:
      LOG(WARNING, "Unknown package received, drop it.");
      break;
  }
}
