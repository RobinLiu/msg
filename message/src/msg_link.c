#include "msg_link.h"
#include "mock_driver.h"
#include "common/base/thread.h"
#include "common/base/timer.h"
#include "common/base/lock.h"
#include "common/base/message.h"
#include "message/client/app_info.h"
#include "message/client/app_config.h"
#include "message_router.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
/******************************************************************************
 * MACRO definitions
 *****************************************************************************/
#define USING_UDP_FOR_MESSAGE 		1
#define MAX_MSG_IN_QUEUE      		8192
#define QUEUE_FULL_WAIT_TIME  		100000  /*0.1s in microseconds*/
#define MAX_UNACKED_HEARTBEAT_NUM 	10

/* Timer values. Unit is ms */
#define RETRANS_TIMEOUT     		2000
#define HEARTBEAT_TIMEOUT   		2500
#define HEARTBEAT_INTERVAL  		3000

/*Link status*/
#define LINK_STATUS_UNKNOWN     0
#define LINK_STATUS_DOWN        1
#define LINK_STATUS_UP          2


/* the ring buffer size, MUST be 2^n */
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
typedef struct msg_frag
{
  uint8* data;
  uint8* ptr_ref;
  uint16 data_len;
  uint16 _reserved;
//  struct list_head list;
} msg_frag_t;

typedef struct
{
   struct msg_frag* frag;
} tx_win_item_t;

#if USING_UDP_FOR_MESSAGE
typedef struct conn_info
{
  int connfd;
  struct sockaddr_in sockinfo;
}conn_info_t;
#endif

typedef struct link
{

  node_id_t         peer;
  msg_timer_t       retrans_timer;
  msg_timer_t       heartbeat_timer;
  msg_timer_t       heartbeat_interval;
  uint8             unacked_heatbeat_num;
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
    //statistics
    uint32          msg_queue_size;
    uint32          num_of_sent_msg;
    uint32          num_of_sent_frag;
    uint32          num_of_acked_frag;
    uint32          num_of_droped_msg;
  } tx;

  struct {
    uint16          exp_frame_seq;
    uint16          requested_frag;
    message_t*      message;
    lock_t          rx_lock;
    //statistics
    uint32          num_of_ack_sent;
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
void start_debug_thread();
void start_heartbeater(void* data);


void init_msg_link()
{
	uint32 i = 0;

	init_lock(&g_link_table_lock);

	lock(&g_link_table_lock);
	for (; i < MAX_NODE_NUM; ++i)
	{
		g_msg_link[i] = NULL;
	}unlock(&g_link_table_lock);
	start_debug_thread();
}

msg_frag_t* alloc_msg_frag(uint32 payload_len)
{

	msg_frag_t* msg_frag = (msg_frag_t*) malloc(sizeof(msg_frag_t));
	CHECK(NULL != msg_frag);

	msg_frag->ptr_ref = (uint8*) malloc(payload_len + TOTAL_HEADER_LEN);
	CHECK(NULL != msg_frag->ptr_ref);

	msg_frag->data = msg_frag->ptr_ref + TOTAL_HEADER_LEN;
	msg_frag->data_len = payload_len;

	return msg_frag;
}

void free_msg_frag(msg_frag_t** msg_frag)
{
	if (NULL == msg_frag || (*msg_frag) == NULL)
	{
		return;
	}

	if (NULL != (*msg_frag)->ptr_ref)
	{
		free((*msg_frag)->ptr_ref);
		(*msg_frag)->ptr_ref = NULL;
	}

	free(*msg_frag);
	(*msg_frag) = NULL;
}

static void fill_msg_to_msg_frag(message_t* msg, msg_frag_t* msg_frag)
{
	if (msg->buf_len > MAX_FRAME_PAYLOAD_LEN)
	{
		msg_frag->data_len = MAX_FRAME_PAYLOAD_LEN;
	}
	else
	{
		msg_frag->data_len = msg->buf_len;
	}

	memcpy((void*) msg_frag->data, (void*) msg->buf_ptr, msg_frag->data_len);
	msg->buf_ptr += msg_frag->data_len;
	msg->buf_len -= msg_frag->data_len;
}

static inline bool sliding_window_is_empty(msg_link_t* link)
{
	return (uint8) (link->tx.win_end == link->tx.win_start);
}

static inline bool sliding_window_is_full(msg_link_t* link)
{
	return (WIN_INDEX_SUB(link->tx.win_end, link->tx.win_start)
			>= g_tx_window_size);
}

static inline uint16 sliding_window_get_used_size(msg_link_t* link)
{
	return WIN_INDEX_SUB(link->tx.win_end, link->tx.win_start);
}

static inline int32 sliding_window_get_ind(uint16 frame_seq, msg_link_t* link)
{
	uint16 in_flight_frames, tail_frames;
	tail_frames = FRAME_SEQ_SUB(link->tx.frame_seq, frame_seq);
	in_flight_frames = sliding_window_get_used_size(link);
	if (tail_frames > in_flight_frames || tail_frames == 0)
	{
		LOG(INFO, "sliding_window_get_ind failed");
		return -1;
	}
	return (int32) WIN_INDEX_SUB(link->tx.win_end, tail_frames);
}

static inline void sliding_window_move_to(uint16 new_win_start,
		msg_link_t* link)
{
	uint16 ind;
	for (ind = link->tx.win_start; ind != new_win_start; ind =
			WIN_INDEX_ADD(ind, 1))
	{
		free_msg_frag(&(link->tx.win[ind].frag));
		link->tx.win[ind].frag = NULL;
	}
	link->tx.win_start = new_win_start;
}

static uint32 get_msg_link_idx(node_id_t node_id)
{
	//TODO: map node id to link index;
	return node_id;
}

static void prepare_pkt(msg_frag_t* msg_frag, node_id_t peer)
{
	//TODO: fill layer2 header here;
	uint8 dest_mac[MAC_ADDR_LEN];
	uint8 self_mac[MAC_ADDR_LEN];
	//get_self_mac(self_mac);
	//get_peer_mac(peer, dest_mac);

	msg_frag->data -= ETH_HEADER_LEN;
	msg_frag->data_len += ETH_HEADER_LEN;
	uint8* data_ptr = msg_frag->data;

	/*ethernet header contains following field:
	 * |dest mac  | src mac|optinal vlan tag| ether type|
	 * |6(octets) |6       |4               | 2         |
	 * */
	//memcpy(data_ptr, dest_mac, MAC_ADDR_LEN);
	//memcpy(data_ptr + MAC_ADDR_LEN, self_mac, MAC_ADDR_LEN);
	data_ptr += MAC_ADDR_LEN * 2;
	*((uint16*) data_ptr) = ETH_P_MSG; //TODO: htons maybe needed.
}

static void conn_xmit_pkt(msg_frag_t* msg_frag, msg_link_t* link)
{
    CHECK(NULL != msg_frag);
    CHECK(NULL != link);
//#if USING_LAYER_2
	prepare_pkt(msg_frag, link->peer);
//#endif
	driver_xmit_pkt(link->conn_info.connfd, (void*) &link->conn_info.sockinfo,
			(void*) msg_frag->data, msg_frag->data_len);
//  LOG(INFO, "xmit frag: %d to node %d", link->tx.frame_seq, link->peer);
}

static void tx_msg_enqueue(message_t* msg, msg_link_t* link)
{
	uint32 queue_size = 0;
	lock(&link->tx.tx_lock);
	queue_size = link->tx.msg_queue_size;
	bool was_empty = list_empty(&link->tx.msg_queue);

	if ((MAX_MSG_IN_QUEUE / 2) <= queue_size && MAX_MSG_IN_QUEUE >= queue_size)
	{
		unlock(&link->tx.tx_lock);
		LOG(WARNING, "Sender thread is going to sleep for %d ms",
				QUEUE_FULL_WAIT_TIME/1000);
		usleep(QUEUE_FULL_WAIT_TIME);
		lock(&link->tx.tx_lock);
	}
	else if (MAX_MSG_IN_QUEUE < queue_size)
	{
		//drop the message and return;
		link->tx.num_of_droped_msg++;
		LOG(ERROR, "Droped msg num %d", link->tx.num_of_droped_msg);
		free_msg_buff(&msg);
		unlock(&link->tx.tx_lock);
		return;
	}

	//TODO: limit the number of message in the queue to avoid too many messages.
	list_add_tail(&msg->list, &link->tx.msg_queue);
	link->tx.msg_queue_size++;
	unlock(&link->tx.tx_lock);
	//LOG(INFO, "queue size is %d for node %d",
			//link->tx.msg_queue_size, link->peer);
	if (was_empty)
	{
		LOG(INFO, "New message to node %d arrived , notify thread...",
				link->peer);
		notify_thread(&link->tx.tx_queue_threshold);
	}
}

static void send_zero_msg(uint16 frame_seq, uint8 msg_type, msg_link_t* link,
		uint32 msg_seq)
{
	msg_frag_t* msg = alloc_msg_frag(0);
	CHECK(NULL != msg);
	link_header_t* lh = (link_header_t*) (msg->data - sizeof(link_header_t));
	lh->version = PROTO_VER;
	lh->pdu_type = msg_type;
	lh->src_node = get_self_node_id();
	lh->dst_node = link->peer;
	lh->frag_seq = frame_seq;
	lh->frag_len = 0;
	lh->frag_index = 0;
	lh->msg_seq = msg_seq;
	lh->is_last_frag = 1;
//  LOG(INFO, "frag_seq is %d to node %d", lh->frag_seq, link->peer);
	msg->data_len += sizeof(link_header_t);
	msg->data -= sizeof(link_header_t);
	conn_xmit_pkt(msg, link);
	free_msg_frag(&msg);
}

#if ENABLE_RESET_LINK
static void send_reset_req(msg_link_t* link)
{
	send_zero_msg(link->rx.exp_frame_seq, PDU_RESET, link, 0);
}

static void send_reset_ack(msg_link_t* link)
{
	send_zero_msg(link->rx.exp_frame_seq, PDU_RESET_ACK, link, 0);
}
#endif

//static uint16 get_start_frame_seq(msg_link_t* link) {
//  if (NULL == link->tx.win[link->tx.win_start].frag ||
//      NULL == link->tx.win[link->tx.win_start].frag->data) {
//    return link->tx.frame_seq;
//  }
//  msg_frag_t* frag = link->tx.win[link->tx.win_start].frag;
//  link_header_t* lh = (link_header_t*)(frag->data + ETH_HEADER_LEN);
//  return lh->frag_seq;
//}

static void send_heartbeat(msg_link_t* link)
{
//  if (link->status != LINK_STATUS_OK) {
	LOG(INFO, "Begin to send HeartBeat to node %d", link->peer);
	//uint16 frag_seq = get_start_frame_seq(link);
	send_zero_msg(/*frag_seq*/link->tx.frame_seq, PDU_HB, link, link->status);
	LOG(INFO, "end send HeartBeat to node %d", link->peer);
//  }
}

static void send_heartbeat_ack(msg_link_t* link)
{
	LOG(INFO, "Begin to send HeartBeat ack to node %d", link->peer);
	//uint16 frag_seq = get_start_frame_seq(link);
//  send_zero_msg(frag_seq, PDU_HB, link, link->status);
	send_zero_msg(/*frag_seq*/link->tx.frame_seq, PDU_HB_ACK, link,
			link->status);
	LOG(INFO, "end send HeartBeat ack to node %d", link->peer);
}

static void send_ack(link_header_t* lh, msg_link_t* link)
{
	LOG(INFO, "ack for seq %d to node %d", lh->frag_seq, link->peer);
	send_zero_msg(lh->frag_seq, PDU_ACK, link, lh->msg_seq);
}

static void send_retrans_req(msg_link_t* link)
{
	send_zero_msg(link->rx.exp_frame_seq, PDU_RETRANS, link, 0);
}

static void send_retrans(uint16 win_ind, msg_link_t* link)
{
	uint16 ind;
	if (win_ind > g_tx_window_size)
	{
		LOG(WARNING, "Invalid windows index: %d to node %d",
				win_ind, link->peer);
		return;
	}
	ind = win_ind;
	for (ind = win_ind; ind != link->tx.win_end; ind = WIN_INDEX_ADD(ind, 1))
	{
		//make sure that the frame buffer has not been freed.
		LOG(INFO, "Retrans wind[%d] to node %d", ind, link->peer);
		CHECK(NULL != link->tx.win[ind].frag);
//    conn_xmit_pkt(link->tx.win[ind].frag, link->peer);
		msg_frag_t* msg_frag = link->tx.win[ind].frag;
		driver_xmit_pkt(link->conn_info.connfd,
				(void*) &link->conn_info.sockinfo, (void*) msg_frag->data,
				msg_frag->data_len);
	}
}

static error_no_t put_msg_into_sliding_window(message_t* msg, msg_link_t* link)
{
	if (NULL == msg || NULL == link)
	{
		LOG(WARNING, "Pass NUll pointer to function");
		return NULL_POINTER_EC;
	}

	bool was_empty = sliding_window_is_empty(link);
	uint16 frag_index = 0;

	while (msg->buf_len > 0)
	{
//    LOG(INFO, "msg->buf_len: %d", msg->buf_len);
		if (sliding_window_is_full(link))
		{
			//should not enter here, since we have already checked with lock before.
			LOG(WARNING, "sliding window is full. Wait to be notified...");
			wait_to_be_notified(&link->tx.tx_queue_threshold,
					&link->tx.tx_lock);
		}
		bool is_last_frag = (msg->buf_len <= MAX_FRAME_PAYLOAD_LEN);
		uint32 payload_len =
				msg->buf_len > MAX_FRAME_PAYLOAD_LEN ?
						MAX_FRAME_PAYLOAD_LEN : msg->buf_len;

		msg_frag_t* msg_frag = alloc_msg_frag(payload_len);
		CHECK(NULL != msg_frag);

		//copy content to msg frag;
		fill_msg_to_msg_frag(msg, msg_frag);

		//get link header ptr
		msg_frag->data -= sizeof(link_header_t);
		msg_frag->data_len += sizeof(link_header_t);
		link_header_t* lh = (link_header_t*) msg_frag->data;

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
		//LOG(INFO, "Send frag %d to node %d by wind[%d]",
				//lh->frag_seq, link->peer, end);
		//move sliding window to next;
		link->tx.win_end = WIN_INDEX_ADD(end, 1);
		//send to driver;
		conn_xmit_pkt(link->tx.win[end].frag, link);
		link->tx.num_of_sent_frag++;
	}

	if (was_empty && !sliding_window_is_empty(link))
	{
		renew_timer(&link->retrans_timer, RETRANS_TIMEOUT);
	}

	return SUCCESS_EC;
}

static void* tx_thread_worker(void* arg)
{
	CHECK(NULL != arg);
	msg_link_t* link = (msg_link_t*) arg;

	message_t* msg = NULL;
	link->status = LINK_STATUS_UP;
//  char big_buf[8*1024*1024 + 2] = {0};
//  send_heartbeat(link);
//  lock(&link->tx.tx_lock);
	start_heartbeater((void*) link);

	while (1)
	{
		lock(&link->tx.tx_lock);
		if (list_empty(&link->tx.msg_queue) || sliding_window_is_full(link) ||
		link->status != LINK_STATUS_UP ||
		link->peer_status != LINK_STATUS_UP){
		wait_to_be_notified(&link->tx.tx_queue_threshold, &link->tx.tx_lock);
	}

		if (link->status != LINK_STATUS_UP
				|| link->peer_status != LINK_STATUS_UP)
		{
			LOG(ERROR, "Link status is not ok for node %d", link->peer);
			unlock(&link->tx.tx_lock);
			//TODO: get peer node status from the node manager, if the status is ok
			//then begin the heart beat.
			if (LINK_STATUS_UNKNOWN == link->peer_status)
			{
//        LOG(INFO, "Send .....");
//        send_heartbeat(link);
				start_heartbeater((void*) link);
			}
			continue;
		}

		if (list_empty(&link->tx.msg_queue))
		{
			unlock(&link->tx.tx_lock);
			continue;
		}

		if (sliding_window_is_full(link))
		{
			LOG(INFO, "sliding window is full");
			if (!is_timer_started(&link->retrans_timer))
			{
				LOG(ERROR, "Retrans Timer for node %d stopped, start it...",
						link->peer);
				start_timer(&link->retrans_timer);
			}unlock(&link->tx.tx_lock);
			continue;
		}

		//get one message from message queue and put it into sliding window;
		msg = list_entry(link->tx.msg_queue.next, message_t, list);
		CHECK(NULL != msg);
		put_msg_into_sliding_window(msg, link);

		//this message has been put to sliding window, delete it
		list_del(&msg->list);
		free_msg_buff(&msg);
		link->tx.msg_queue_size--;
		link->tx.num_of_sent_msg++;
//    LOG(INFO, "Send the %dth msg to node %d",
//              link->tx.num_of_sent_msg, link->peer);
		unlock(&link->tx.tx_lock);
	}

	return (void*) (0);
}

void timeout_handle(void* data)
{
	CHECK(NULL != data);
	msg_link_t* link = (msg_link_t*) data;

	LOG(WARNING, "Timeout for msg ack to node %d, retrans it the %d times.",
			link->peer, link->tx.retrans_timeout_count);
	lock(&link->tx.tx_lock);
	if (link->tx.retrans_timeout_count < MAX_RETRNS_COUNT)
	{
		send_retrans(link->tx.win_start, link);
		link->tx.retrans_timeout_count++;
		renew_timer(&link->retrans_timer, RETRANS_TIMEOUT);
//    renew_timer(&link->retrans_timer, RETRANS_TIMEOUT);
	}
	else
	{
		//TODO: maybe need to set the status of the link unavailable
		link->peer_status = LINK_STATUS_DOWN;
		LOG(ERROR, "Max retry count reached, set the node %d as unusable.",
				link->peer);
		stop_timer(&link->retrans_timer);
//    //drop the frag and move to next frag.
//    uint16 new_win_start = WIN_INDEX_ADD(link->tx.win_start, 1);
//    //del msg_frag from sliding window
//    sliding_window_move_to(new_win_start, link);
		link->tx.retrans_timeout_count = 0;
	}
//  bool is_empty = sliding_window_is_empty(link);
//  if (!is_empty) {
//  }
	LOG(INFO, "Out timeout_handle for node %d", link->peer);
	unlock(&link->tx.tx_lock);
}

void start_heartbeater(void* data)
{
	CHECK(NULL != data);
	msg_link_t* link = (msg_link_t*) data;
	lock(&link->tx.tx_lock);
	send_heartbeat(link);
	start_timer(&link->heartbeat_timer);
	unlock(&link->tx.tx_lock);
//  start_timer(&link->heartbeat_interval);
}

void heartbeat_timeout_handle(void* data)
{
	CHECK(NULL != data);
	msg_link_t* link = (msg_link_t*) data;
	lock(&link->lock);
	link->unacked_heatbeat_num++;
	if (MAX_UNACKED_HEARTBEAT_NUM <= link->unacked_heatbeat_num)
	{
		link->peer_status = LINK_STATUS_DOWN;
		unlock(&link->lock);
	}
	else
	{
		unlock(&link->lock);
		start_heartbeater((void*) link);
	}
}

static void get_conn_info(node_id_t node_id, conn_info_t* conn_info)
{

	int8 serv_ip_addr[32] =
	{ 0 };
	uint16 serv_port = 0;
	get_serv_info(node_id, serv_ip_addr, &serv_port);
	memset(&conn_info->sockinfo, 0, sizeof(conn_info->sockinfo));
	conn_info->sockinfo.sin_family = AF_INET;
	conn_info->sockinfo.sin_port = htons(serv_port);
	inet_pton(AF_INET, (char*) serv_ip_addr,
			(void*) &conn_info->sockinfo.sin_addr);
	//inet_aton

	conn_info->connfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	CHECK(-1 != conn_info->connfd);
}

static void init_link(node_id_t node_id, msg_link_t* link)
{
	INIT_LIST_HEAD(&link->tx.msg_queue);
	init_lock(&link->lock);
	init_lock(&link->tx.tx_lock);
	init_lock(&link->rx.rx_lock);
	get_conn_info(node_id, &link->conn_info);
	thread_cond_init(&link->tx.tx_queue_threshold);
	link->peer = node_id;
	link->status = LINK_STATUS_DOWN;
	link->peer_status = LINK_STATUS_UNKNOWN;

	init_timer(&link->retrans_timer, &timeout_handle, (void*) link,
			RETRANS_TIMEOUT);
	init_timer(&link->heartbeat_interval, &start_heartbeater, (void*) link,
			HEARTBEAT_INTERVAL);
	init_timer(&link->heartbeat_timer, &heartbeat_timeout_handle, (void*) link,
			HEARTBEAT_TIMEOUT);
	LOG(INFO, "Timer for node %d are inited", link->peer);
	create_thread(&link->tx.tx_thread_id, NULL, tx_thread_worker, (void*) link);
}

msg_link_t* get_msg_link(node_id_t node_id)
{
	lock(&g_link_table_lock);
	uint32 link_idx = get_msg_link_idx(node_id);
//  LOG(INFO, "link_idx is %d", link_idx);
	if (link_idx >= MAX_NODE_NUM)
	{
		LOG(WARNING, "Get msg link faild");
		unlock(&g_link_table_lock);
		return NULL;
	}

	if (g_msg_link[link_idx] != NULL)
	{
		if (g_msg_link[link_idx]->conn_info.connfd == -1)
		{
			get_conn_info(g_msg_link[link_idx]->peer,
					&g_msg_link[link_idx]->conn_info);
		}unlock(&g_link_table_lock);
		return g_msg_link[link_idx];
	}

	//new connection
	msg_link_t* new_link = (msg_link_t*) malloc(sizeof(msg_link_t));
	CHECK(NULL != new_link);
	memset(new_link, 0, sizeof(msg_link_t));

	//TODO: initialization for the new link.
	init_link(node_id, new_link);
	g_msg_link[link_idx] = new_link;
	unlock(&g_link_table_lock);
	return g_msg_link[link_idx];
}

error_no_t send_msg_to_node(message_t* msg, node_id_t dest_node)
{
	msg_link_t* msg_link = NULL;
	msg_link = get_msg_link(dest_node);
	if (msg_link == NULL)
	{
		LOG(WARNING, "Get msg link failed");
		return GET_MSG_LINK_FAILED_EC;
	}
	tx_msg_enqueue(msg, msg_link);

	return SUCCESS_EC;
}

#if ENABLE_RESET_LINK
static void handle_reset(uint8* pkt, msg_link_t* link)
{
	link_header_t* lh = (link_header_t*)(pkt + ETH_HEADER_LEN);

	link->status = LINK_STATUS_INIT;

	lock(&link->tx.tx_lock);
	//reset all the fragement on the go;
	int32 i = 0;
	for(i = 0; i < g_tx_window_size; ++i)
	{
		if (link->tx.win[i].frag != NULL )
		{
			free_msg_frag(&(link->tx.win[i].frag));
			link->tx.win[i].frag = NULL;
		}
	}
	link->tx.win_start = 0;
	link->tx.win_end = 0;
	link->tx.frame_seq = 0;

	//deal with half sent message;
	//TODO: how about the enqueued message which has not been sent out.
	if (!list_empty(&link->tx.msg_queue))
	{
		message_t* msg = list_entry(link->tx.msg_queue.next, message_t, list);
		CHECK(NULL != msg);
		if (msg->buf_head != msg->buf_ptr)
		{
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

static void handle_reset_ack(uint8* pkt, msg_link_t* link)
{
	//send_heartbeat(link);
}
#endif

static void handle_heartbeat(uint8* pkt, msg_link_t* link)
{

	lock(&link->lock);
	lock(&link->rx.rx_lock);
	LOG(INFO, "HeartBeat is received from node %d", link->peer);
	link_header_t* lh = (link_header_t*) pkt;
	link->rx.exp_frame_seq = lh->frag_seq;
	//borrow the msg_seq to represent the status of peer.
//  uint32 peer_status = lh->msg_seq;
	//if peer status is
	link->peer_status = LINK_STATUS_UP;
	send_heartbeat_ack(link);
	link->status = LINK_STATUS_UP;
	unlock(&link->rx.rx_lock);
	unlock(&link->lock);
}

static void handle_heartbeat_ack(uint8* pkt, msg_link_t* link)
{
	lock(&link->lock);
	lock(&link->rx.rx_lock);
	stop_timer(&link->heartbeat_timer);
	LOG(INFO, "HeartBeat ack is received from node %d", link->peer);
	link_header_t* lh = (link_header_t*) pkt;
	link->peer_status = lh->msg_seq;
	link->rx.exp_frame_seq = lh->frag_seq;
//  link->status = LINK_STATUS_UP;
//  link->peer_status = LINK_STATUS_UP;

	if (LINK_STATUS_UP != link->peer_status)
	{
		link->peer_status = LINK_STATUS_UP;
		notify_thread(&link->tx.tx_queue_threshold);
		LOG(
				INFO,
				"Peer is ready to receive packet now, notify thread to send...");
	} /*else {
	 LOG(INFO, "peer is not ok, send HeartBeat again");
	 send_heartbeat(link);
	 }*/
	unlock(&link->rx.rx_lock);
	unlock(&link->lock);
}

static void handle_data(uint8* pkt, msg_link_t* link)
{
	link_header_t* lh = (link_header_t*) pkt;
	lock(&link->rx.rx_lock);
	if (lh->frag_seq == link->rx.exp_frame_seq)
	{
		LOG(INFO, "Correct seq received from node %d, seq is %d",
				link->peer, lh->frag_seq);
		//send ack to peer
		link->rx.exp_frame_seq++;
		link->rx.num_of_rcvd_frag++;
		send_ack(lh, link);
		link->rx.num_of_ack_sent++;

		link->rx.requested_frag = 0;
		if (lh->frag_index == 0)
		{
			//first frame of a message, read header and alloc message buf;
//      LOG(INFO, "lh->frag_len %d sizeof(msg_header_t) %d.", lh->frag_len, sizeof(msg_header_t));
			CHECK(lh->frag_len >= sizeof(msg_header_t));
			msg_header_t* msg_header = (msg_header_t*) (pkt + sizeof(link_header_t));
			uint32 msg_len = msg_header->msg_len;

			message_t* new_msg = allocate_msg_buff(msg_len);
			CHECK(NULL != new_msg);
			if (NULL != link->rx.message)
			{
				//TODO: maybe should move the free to router, to make sure that the
				//message has been delivered to user's program.
				free_msg_buff(&link->rx.message);
				link->rx.message = NULL;
			}
			//set the buf that has been copied as 0;
			check_buff_magic_num(new_msg);
			new_msg->buf_len = 0;
			link->rx.message = new_msg;
		}
		else if (link->rx.message == NULL
				|| link->rx.message->header->msg_seq != lh->msg_seq)
		{
			LOG(WARNING,
				"First frame of a message is not received from node %d, drop the fragment",
				link->peer);
			unlock(&link->rx.rx_lock);
			//CHECK(0 == 1);
			return;
		}
		uint8* msg_data = (uint8*) (pkt + sizeof(link_header_t));
		memcpy(link->rx.message->buf_ptr, msg_data, lh->frag_len);

		link->rx.message->buf_ptr += lh->frag_len;
		link->rx.message->buf_len += lh->frag_len;

		if (lh->is_last_frag)
		{
			//this is the last of the frag;
			check_buff_magic_num(link->rx.message);
			uint32 total_len = sizeof(msg_header_t)
					+ link->rx.message->header->msg_len + sizeof(msg_tail_t);
//      LOG(INFO,"total_len %d, received len %d", total_len, link->rx.message->buf_len);
			CHECK(link->rx.message->buf_len == total_len);
			//send message to router
			router_receive_msg(link->rx.message);
			link->rx.message = NULL;
			link->rx.num_of_rcvd_msg++;
			LOG(INFO, "received %d msg from node %d",
					link->rx.num_of_rcvd_msg, link->peer);
		}
	}
	else if (FRAME_SEQ_GT(lh->frag_seq , link->rx.exp_frame_seq))
	{
		if (link->rx.requested_frag != link->rx.exp_frame_seq)
		{
			//unexpectd frame received, ignore or request retrans
			LOG(WARNING, "Retrans reques for frame seq: %d to node %d",
					link->rx.exp_frame_seq, link->peer);
			send_retrans_req(link);
			link->rx.requested_frag = link->rx.exp_frame_seq;
		}

	}
	else
	{ //duplicated buff received, send ack to peer and drop the packet
		//CHECK(0 == 1);
		LOG(
				WARNING,
				"Duplicated package received from node %d ,send ack and drop it",
				link->peer);
		send_ack(lh, link);
		link->rx.num_of_ack_sent++;
	}
	unlock(&link->rx.rx_lock);
}

static void handle_ack(void* pkt, msg_link_t* link)
{
//  LOG(INFO, "ack is received.");
	link_header_t* lh = (link_header_t*) pkt;

	lock(&link->tx.tx_lock);
	bool was_full = sliding_window_is_full(link);
	int32 win_ind_ret = sliding_window_get_ind(lh->frag_seq, link);
	if (win_ind_ret < 0)
	{
		LOG(WARNING, "Incorrect ack received from node %d, drop it",
				link->peer);
//    send_heartbeat(link);
		unlock(&link->tx.tx_lock);
		return;
	}

	LOG(INFO, "ack is received for frag:%d from node %d",
			lh->frag_seq, link->peer);
	uint16 win_ind = (uint16) win_ind_ret;
	uint16 new_win_start = WIN_INDEX_ADD(win_ind, 1);
	//del msg_frag from sliding window
	sliding_window_move_to(new_win_start, link);
	link->tx.retrans_timeout_count = 0;
	link->tx.num_of_acked_frag++;

	LOG(INFO, "num_of_acked_frag %d from node %d",
			link->tx.num_of_acked_frag, link->peer);
//  if(link->tx.num_of_acked_frag == 100000) {
//    exit(0);
//  }
	//Notify sender to continuE send if sliding window was full;
	if (was_full)
	{
		LOG(
				INFO,
				"Sliding window has space now, notify thread to send packet...");
		notify_thread(&link->tx.tx_queue_threshold);
	}
	bool is_empty = sliding_window_is_empty(link);

	if (!is_empty)
	{
		renew_timer(&link->retrans_timer, RETRANS_TIMEOUT);
//    start_timer(&link->retrans_timer);
	}
	else
	{
		//LOG(INFO, "Stop Retrans timer for node %d", link->peer);
		stop_timer(&link->retrans_timer);
	}
	unlock(&link->tx.tx_lock);
}

static void handle_retrans_req(void* pkt, msg_link_t* link)
{
	lock(&link->tx.tx_lock);
	link_header_t* lh = (link_header_t*) pkt;
	int32 win_ind_ret = sliding_window_get_ind(lh->frag_seq, link);
	if (win_ind_ret < 0)
	{
		LOG(WARNING, "Incorrect retrans req received from node %d, drop it",
				link->peer);
//    CHECK(win_ind_ret >= 0);
//    send_heartbeat(link);
		unlock(&link->tx.tx_lock);
		start_heartbeater((void*) link);
		return;
	}
	uint16 win_ind = (uint16) win_ind_ret;

	send_retrans(win_ind, link);
	unlock(&link->tx.tx_lock);
}

static bool is_pkt_valid(void* pkt, uint32 pkt_len)
{
	if (pkt == NULL || pkt_len < TOTAL_HEADER_LEN)
	{
		return FALSE;
	}
	//TODO: other check;

	return TRUE;
}

uint8* strip_eth_header(void* pkt)
{
	uint8* frag_ment = (uint8*) ((uint8*) pkt + ETH_HEADER_LEN);
	return frag_ment;
}

void on_pkt_received(void* pkt, uint32 pkt_len)
{
	if (!is_pkt_valid(pkt, pkt_len))
	{
		LOG(WARNING, "Package received, but failed to pass the check, drop it.");
		return;
	}

	uint8* frag_ment = strip_eth_header(pkt);
	link_header_t* lh = (link_header_t*) (frag_ment);
	msg_link_t* link = get_msg_link(lh->src_node);

	if (NULL == link)
	{
		LOG(WARNING, "Get link for message failed, node id is %d, drop the message.",
			lh->src_node);
		return;
	}

	stop_timer(&link->heartbeat_interval);
//  if (LINK_STATUS_UP != link->peer_status) {
//    start_heartbeater((void*)link);
//  }

	switch (lh->pdu_type)
	{
#if  ENABLE_RESET_LINK
	case PDU_RESET:
	handle_reset(frag_ment, link);
	break;
	case PDU_RESET_ACK:
	handle_reset_ack(frag_ment, link);
	break;
#endif
	case PDU_HB:
		handle_heartbeat(frag_ment, link);
		break;
	case PDU_HB_ACK:
		handle_heartbeat_ack(frag_ment, link);
		break;
	case PDU_DATA:
		if (LINK_STATUS_UP == link->status
				&& LINK_STATUS_UP == link->peer_status)
		{
			handle_data(frag_ment, link);
		}
		else
		{
			LOG(INFO, "Data received from node %d, set peer state as usable.",
					link->peer);
			start_heartbeater((void*) link);
//        link->peer_status = LINK_STATUS_UP;
//        notify_thread(&link->tx.tx_queue_threshold);
		}
		break;
	case PDU_ACK:
		if (LINK_STATUS_UP == link->status
				&& LINK_STATUS_UP == link->peer_status)
		{
			handle_ack(frag_ment, link);
		}
		else
		{
			LOG(INFO, "Data received from node %d, set peer state as usable.",
					link->peer);
			start_heartbeater((void*) link);
//        link->peer_status = LINK_STATUS_UP;
//        notify_thread(&link->tx.tx_queue_threshold);
		}
		break;
	case PDU_RETRANS:
		if (LINK_STATUS_UP == link->status
				&& LINK_STATUS_UP == link->peer_status)
		{
			handle_retrans_req(frag_ment, link);
		}
		else
		{
			LOG(INFO, "Data received from node %d, set peer state as usable.",
					link->peer);
//        link->peer_status = LINK_STATUS_UP;
//        notify_thread(&link->tx.tx_queue_threshold);
			start_heartbeater((void*) link);
		}
		break;
	default:
		LOG(WARNING, "Unknown package received, drop it.");
		break;
	}
	start_timer(&link->heartbeat_interval);
}

void node_state_changed(node_id_t node, uint8 status)
{
	LOG(WARNING, "node %d status changed to %d", node, status);
	msg_link_t* link = get_msg_link(node);
	if (NULL == link)
	{
		return;
	}lock(&link->lock);
//  link->peer_status = status;
	if (LINK_STATUS_UP == status)
	{
		//set to this status so that we can start to do the heartbeat;
		link->peer_status = LINK_STATUS_UNKNOWN;
		LOG(INFO, "Node state changed to ok, notify the thread...");
		notify_thread(&link->tx.tx_queue_threshold);
	}
	else
	{
		LOG(INFO, "link went down");
		//explicitly set the status as down so that heartbeat will not be sent.
		link->peer_status = LINK_STATUS_DOWN;
		//stop all timers(activities)
		stop_timer(&link->heartbeat_timer);
		stop_timer(&link->retrans_timer);
		stop_timer(&link->heartbeat_interval);
	}
	unlock(&link->lock);
}

void print_link_info(int index)
{
	if (g_msg_link[index] == NULL)
	{
		printf("No data on link %d\n", index);
		return;
	}
	printf("peer                           %d\n", g_msg_link[index]->peer);
	printf("retrans_timer.timer_id         %d\n",
			g_msg_link[index]->retrans_timer.timer_id);
	printf("retrans_timer.delay.sec        %ld\n",
			g_msg_link[index]->retrans_timer.delay.sec);
	printf("retrans_timer.delay.msec       %ld\n",
			g_msg_link[index]->retrans_timer.delay.msec);
	printf("retrans_timer.expires in       %d microseconds\n\n",
			timer_will_expire_in(&g_msg_link[index]->retrans_timer));
	printf("heartbeat_timer.timer_id       %d\n",
			g_msg_link[index]->heartbeat_timer.timer_id);
	printf("heartbeat_timer.delay.sec      %ld\n",
			g_msg_link[index]->heartbeat_timer.delay.sec);
	printf("heartbeat_timer.delay.msec     %ld\n",
			g_msg_link[index]->heartbeat_timer.delay.msec);
	printf("heartbeat_timer.expires in     %d microseconds\n\n",
			timer_will_expire_in(&g_msg_link[index]->heartbeat_timer));
	printf("heartbeat_interval.timer_id    %d\n",
			g_msg_link[index]->heartbeat_interval.timer_id);
	printf("heartbeat_interval.delay.sec   %ld\n",
			g_msg_link[index]->heartbeat_interval.delay.sec);
	printf("heartbeat_interval.delay.msec  %ld\n",
			g_msg_link[index]->heartbeat_interval.delay.msec);
	printf("heartbeat_interval.expires in  %d microseconds\n\n",
			timer_will_expire_in(&g_msg_link[index]->heartbeat_interval));
	printf("unacked_heatbeat_num           %d\n",
			g_msg_link[index]->unacked_heatbeat_num);
	printf("status                         %d\n", g_msg_link[index]->status);
	printf("peer_status                    %d\n\n",
			g_msg_link[index]->peer_status);
	printf("tx.frame_seq                   %d\n",
			g_msg_link[index]->tx.frame_seq);
	printf("tx.retrans_timeout_count       %d\n",
			g_msg_link[index]->tx.retrans_timeout_count);
	printf("tx.win_start                   %d\n",
			g_msg_link[index]->tx.win_start);
	printf("tx.win_end                     %d\n",
			g_msg_link[index]->tx.win_end);
	printf("tx.msg_queue_size              %d\n",
			g_msg_link[index]->tx.msg_queue_size);
	printf("tx.num_of_sent_msg             %d\n",
			g_msg_link[index]->tx.num_of_sent_msg);
	printf("tx.num_of_sent_frag            %d\n",
			g_msg_link[index]->tx.num_of_sent_frag);
	printf("tx.num_of_acked_frag           %d\n\n",
			g_msg_link[index]->tx.num_of_acked_frag);
	printf("tx window full                 %d\n",
			sliding_window_is_full(g_msg_link[index]));
	printf("tx window empty                %d\n\n",
			sliding_window_is_empty(g_msg_link[index]));
	printf("rx.exp_frame_seq               %d\n",
			g_msg_link[index]->rx.exp_frame_seq);
	printf("rx.requested_frag              %d\n",
			g_msg_link[index]->rx.requested_frag);
	printf("rx.num_of_ack_sent             %d\n",
			g_msg_link[index]->rx.num_of_ack_sent);
	printf("rx.num_of_rcvd_frag            %d\n",
			g_msg_link[index]->rx.num_of_rcvd_frag);
	printf("rx.num_of_rcvd_msg             %d\n\n",
			g_msg_link[index]->rx.num_of_rcvd_msg);
}

void* debug_thread(void* arg)
{
	(void) arg;
	char input[16] =
	{ 0 };
	int index = 0;
	int choice;
	int log_level = 0;
	char log_filter[256] =
	{ 0 };

	while (1)
	{
		choice = -1;
		memset(input, 0, sizeof(input));
		printf("Input your choice:\n");
		printf("1: Set Log Level.\n");
		printf("2: Set Log filter and level.\n");
		printf("3: Clear Log filter.\n");
		printf("4: Print link info\n");
		printf("5: Exit program\n");
		printf("6: Generate core dump file\n");
		scanf("%s", input);
//    scanf("%d", &choice);
		choice = atoi(input);
		printf("\n");

		switch (choice)
		{
		case 1:
			printf("Set Log level to:\n");
			printf("0: All log\n");
			printf("1: Debug log\n");
			printf("2: Warning log\n");
			printf("3: Error log\n");
			printf("4: Fatal log \n");
			scanf("%d", &log_level);
			set_min_log_level(log_level);
			break;
		case 2:
			printf("Set log filter\n");
			printf("Input filter:\n");
			memset(log_filter, 0, sizeof(log_filter));
			scanf("%s", log_filter);
			set_log_filter(log_filter);
			break;
		case 3:
			clear_log_filter();
			break;
		case 4:
			printf("Input the index of the link:\n");
			scanf("%d", &index);
			if (index < 0 || index >= MAX_NODE_NUM)
			{
				printf("input 0 to %d to get link info\n", MAX_NODE_NUM);
				continue;
			}
			else
			{
				print_link_info(index);
			}
			break;
		case 5:
			exit(0);
			break;
		case 6:
			//used to generate core dump file.
			abort();
			break;
		default:
			break;
		}
	}
}

void start_debug_thread()
{
	pthread_t dbg_thread_id;
	pthread_create(&dbg_thread_id, NULL, &debug_thread, NULL);
}
