#include "msg_client.h"
#include "msg_queue.h"
#include "lock.h"
#include "common/include/common_list.h"
#include "message.h"
#include <unistd.h>
#include <poll.h>

typedef struct msg_client_t
{
	struct list_head sync_msg_pair_list;
	struct list_head aync_msg_list;
	lock_t sync_msg_list_lock;
	lock_t async_msg_list_lock;
	int fd[2]; //pipe used for notification between threads.
} msg_client_t;

typedef struct sync_msg_pair
{
	struct list_head list;
	message_t* msg_req;
	message_t* msg_rsp;
	int fd[2];
} sync_msg_pair_t;

msg_client_t g_msg_client;

msg_client_t* get_msg_client()
{
	return &g_msg_client;
}

void init_msg_client(msg_client_t* msg_client)
{
	CHECK(NULL != msg_client);
	memset(msg_client, 0, sizeof(msg_client_t));
	INIT_LIST_HEAD(&msg_client->sync_msg_pair_list);
	INIT_LIST_HEAD(&msg_client->aync_msg_list);
	init_lock(&msg_client->sync_msg_list_lock);
	init_lock(&msg_client->async_msg_list_lock);
	CHECK(0 == pipe(msg_client->fd));

	start_msg_queue((void*) msg_client);
}

void init_msg_system()
{
	return init_msg_client(&g_msg_client);
}

int is_correct_rsp_msg(message_t* msg_req, message_t* msg_rsp)
{
	CHECK(NULL != msg_req);
	CHECK(NULL != msg_rsp);
	int is_same_sender = msg_req->header->snder.group_id
			== msg_rsp->header->rcver.group_id
			&& msg_req->header->snder.app_id == msg_rsp->header->rcver.app_id;

	int is_same_receiver = msg_req->header->rcver.group_id
			== msg_rsp->header->snder.group_id
			&& msg_req->header->rcver.app_id == msg_rsp->header->snder.app_id;

	int is_same_msg_seq = (msg_req->header->msg_seq == msg_rsp->header->ack_seq);

	return (is_same_sender && is_same_receiver && is_same_msg_seq);
}

void notify_sync_msg_caller(sync_msg_pair_t* sync_msg_pair)
{
	CHECK(NULL != sync_msg_pair);
	CHECK(1 == write(sync_msg_pair->fd[1], "0", 1));
}

void put_rsp_msg_to_sync_queue(message_t* msg)
{
	CHECK(NULL != msg);
	msg_client_t* msg_client = get_msg_client();
	CHECK(NULL != msg_client);

	struct list_head* plist = NULL;
	sync_msg_pair_t* iter = NULL;
	bool bReqfound = FALSE;
	lock(&msg_client->sync_msg_list_lock);
	list_for_each(plist, &msg_client->sync_msg_pair_list)
	{
		iter = list_entry(plist, sync_msg_pair_t, list);
		CHECK(NULL != iter);
		if (is_correct_rsp_msg(iter->msg_req, msg))
		{
			//notify the caller of the sync message.
			iter->msg_rsp = msg;
			bReqfound = TRUE;
			notify_sync_msg_caller(iter);
			LOG(INFO, "Correct sync msg received, notify the caller");
			break;
		}
	}

	if (!bReqfound)
	{
		//If there's no original req found, there are two possibilties:
		//1. The orignal call timed out and remove the req from the queue;
		//2. A wrong message is received.
		// in those cases, we just throw away the message and write a log.
		LOG(
				WARNING,
				"A sync response message is received, but the caller timed out.");
		free_msg_buff(&msg);
	}
	unlock(&msg_client->sync_msg_list_lock);
}

void notify_msg_reader(msg_client_t* msg_client)
{
	CHECK(NULL != msg_client);
	CHECK(1 == write(msg_client->fd[1], "0", 1));
}

void put_msg_to_async_queue(message_t* msg)
{
	CHECK(NULL != msg);
	msg_client_t* msg_client = get_msg_client();
	CHECK(NULL != msg_client);
	lock(&msg_client->async_msg_list_lock);
	bool was_empty = list_empty(&msg_client->aync_msg_list);
	list_add_tail(&msg_client->aync_msg_list, &msg->list);
	if (was_empty)
	{
		notify_msg_reader(msg_client);
	}
	unlock(&msg_client->async_msg_list_lock);
}

int get_first_msg(msg_client_t* msg_client, message_t** msg)
{
	CHECK(NULL != msg_client);
	CHECK(NULL != msg);

	int num_of_msg_got = 0;
	message_t* received_msg = NULL;
	lock(&msg_client->async_msg_list_lock);
	bool is_empty = list_empty(&msg_client->aync_msg_list);
//  LOG(INFO, "is_empty %d", is_empty);
	if (!is_empty)
	{
		received_msg =
				list_entry(msg_client->aync_msg_list.next, message_t, list);
		CHECK(NULL != received_msg);
		list_del(&received_msg->list);
		*msg = received_msg;
		num_of_msg_got = 1;
	}unlock(&msg_client->async_msg_list_lock);
	return num_of_msg_got;
}

int receive_msg_by_client(msg_client_t* msg_client, message_t** msg)
{
	CHECK(NULL != msg_client);
	CHECK(NULL != msg);
	int num_of_msg_received = 0;
	int time_out = -1; //milliseconds;
	num_of_msg_received = get_first_msg(msg_client, msg);
	if (1 == num_of_msg_received)
	{
		return num_of_msg_received;
	}

	struct pollfd pfd =
	{ msg_client->fd[0], POLLIN, 0 };
	char flag[1] =
	{ 0 };
	int ret = poll(&pfd, 1, time_out);
	if (0 == ret)
	{
		LOG(INFO, "timeout, read msg from async queue...");
		num_of_msg_received = get_first_msg(msg_client, msg);
	}
	else if (1 == ret)
	{
		LOG(INFO, "Notified, read msg from async queue...");
		CHECK(1 == read(msg_client->fd[0], flag, 1));
		num_of_msg_received = get_first_msg(msg_client, msg);
	}
	else
	{
		LOG(INFO, "%s", strerror(errno));
	}

	return num_of_msg_received;
}

void receive_msg(message_t** msg)
{
	while (0 == receive_msg_by_client(&g_msg_client, msg))
	{
		continue;
	}
}

void enqueue_msg_to_list(sync_msg_pair_t* sync_msg_pair,
		msg_client_t* msg_client)
{
	CHECK(NULL != sync_msg_pair);
	CHECK(NULL != msg_client);
	lock(&msg_client->sync_msg_list_lock);
	list_add_tail(&msg_client->sync_msg_pair_list, &sync_msg_pair->list);
	unlock(&msg_client->sync_msg_list_lock);
}

int wait_for_rsp(int time_out, sync_msg_pair_t* sync_msg_pair)
{

	CHECK(NULL != sync_msg_pair);
	struct pollfd pfd = { sync_msg_pair->fd[0], POLLIN, 0 };
	char flag[1] = { 0 };
	int ret = poll(&pfd, 1, time_out);
	if (0 == ret)
	{
		LOG(INFO, "Poll time out");
	}
	else if (1 == ret)
	{
		CHECK(1 == read(sync_msg_pair->fd[0], flag, 1));
	}
	else
	{
		LOG(INFO, "%s", strerror(errno));
	}
	return ret;
}

error_no_t send_msg_to_msg_queue(message_t* msg)
{
	msg_queue_id_t msg_queue_id = get_queue_id();
	return msg_queue_send(msg_queue_id, (void*) msg->buf_head,
			(size_t) msg->buf_len, msg->header->priority, RETRY_TIMES);
}

error_no_t send_msg(uint8 dst_grp_id, uint16 dst_app_id, uint32 msg_id,
		uint8 msg_priority, void* msg_content, uint32 msg_len)
{

	message_t* msg = NULL;
	msg = allocate_msg_buff(msg_len);
	CHECK(NULL != msg);

	fill_msg_header(dst_grp_id, dst_app_id, RANGE_ACTIVE, msg_priority,
			MSG_TYPE_ASYNC_MSG, msg_id, msg);

	memcpy((void*) msg->body, msg_content, msg_len);

	error_no_t ret = send_msg_to_msg_queue(msg);
	free_msg_buff(&msg);
	return ret;
}

int send_sync_msg_by_client(msg_client_t* msg_client, message_t* msg,
		int time_out, void* rsp_buf, uint32 rsp_buf_size)
{

	CHECK(NULL != msg_client);
	CHECK(NULL != msg);
	CHECK(NULL != rsp_buf);
	CHECK(time_out > 0);
	CHECK(rsp_buf_size > 0);

	int ret = -1;
	sync_msg_pair_t* sync_msg_pair = (sync_msg_pair_t*) malloc(
			sizeof(sync_msg_pair_t));
	CHECK(NULL != sync_msg_pair);
	sync_msg_pair->msg_req = msg;
	sync_msg_pair->msg_rsp = NULL;
	INIT_LIST_HEAD(&sync_msg_pair->list);
	if (0 != pipe(sync_msg_pair->fd))
	{
		LOG(ERROR, "pipe failed: %s", strerror(errno));
		free(sync_msg_pair);
		sync_msg_pair = NULL;
		return errno;
	}

	enqueue_msg_to_list(sync_msg_pair, msg_client);
	if (0 != send_msg_to_msg_queue(msg))
	{
		LOG(WARNING, "Send message failed");
		lock(&msg_client->sync_msg_list_lock);
		list_del(&sync_msg_pair->list);
		unlock(&msg_client->sync_msg_list_lock);
		close(sync_msg_pair->fd[0]);
		close(sync_msg_pair->fd[1]);
		free(sync_msg_pair);
		sync_msg_pair = NULL;
		return SEND_MSG_TO_MSG_QUEUE_FAILED_EC;
	}

//  LOG(INFO, "Send msg to mq done");
	wait_for_rsp(time_out, sync_msg_pair);

	if (NULL != sync_msg_pair->msg_rsp)
	{
		uint32 buf_size =
				(rsp_buf_size > sync_msg_pair->msg_rsp->header->msg_len) ?
						sync_msg_pair->msg_rsp->header->msg_len : rsp_buf_size;
		memcpy(rsp_buf, sync_msg_pair->msg_rsp->body, buf_size);
		ret = 0;
	}

	//destroy the message pair;
	lock(&msg_client->sync_msg_list_lock);
	list_del(&sync_msg_pair->list);
	free_msg_buff(&sync_msg_pair->msg_req);
	if (NULL != sync_msg_pair->msg_rsp)
	{
		free_msg_buff(&sync_msg_pair->msg_rsp);
	}
	close(sync_msg_pair->fd[0]);
	close(sync_msg_pair->fd[1]);
	free(sync_msg_pair);
	unlock(&msg_client->sync_msg_list_lock);

	return ret;
}

error_no_t send_sync_msg(uint8 dst_grp_id, uint16 dst_app_id, uint32 msg_id,
		uint8 msg_priority, void* msg_content, uint32 msg_len, uint32 time_out,
		void* rsp_buf, uint32 rsp_buf_size)
{

	message_t* msg = NULL;
	msg = allocate_msg_buff(msg_len);
	CHECK(NULL != msg);

	fill_msg_header(dst_grp_id, dst_app_id, RANGE_ACTIVE, msg_priority,
			MSG_TYPE_ASYNC_MSG, msg_id, msg);

	memcpy((void*) msg->body, msg_content, msg_len);

	return send_sync_msg_by_client(&g_msg_client, msg, time_out, rsp_buf,
			rsp_buf_size);
}

error_no_t send_sync_msg_rsp(message_t* msg, void* rsp_buf, int buf_size)
{
	CHECK(NULL != msg);
	CHECK(NULL != rsp_buf);
	CHECK(buf_size > 0);
	message_t* rsp_msg = allocate_msg_buff(buf_size);
	CHECK(NULL != rsp_msg);
	fill_msg_header(msg->header->snder.group_id, msg->header->snder.app_id,
			msg->header->snder.role, msg->header->priority, MSG_TYPE_SYNC_RSP,
			(msg->header->msg_id + 1), rsp_msg);
	rsp_msg->header->ack_seq = msg->header->msg_seq;
	memcpy(rsp_msg->body, rsp_buf, buf_size);
//  print_msg_header(rsp_msg);
	error_no_t ret = send_msg_to_msg_queue(rsp_msg);
	free_msg_buff(&rsp_msg);
	return ret;
}

