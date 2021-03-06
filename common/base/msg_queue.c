#include "msg_queue.h"
#include "thread.h"
#include "message.h"
//#include "msg_client.h"
#include "common/include/common.h"
#include "message/client/app_info.h"

#include <string.h>
#include <errno.h>
#include <time.h>

#define FILE_MODE               0664
#define QUEUE_NAME_LEN          128
#define CLIENT_MSG_QUEUE_SIZE   10
#define SERVER_MSG_QUEUE_SIZE   10
#define MSG_SEND_TIME           500000000  //0.5s In nanoseconds

msg_queue_id_t  g_msg_queue_id = -1;
thread_id_t     g_receiver_thread_id = 0;

extern void put_rsp_msg_to_sync_queue(message_t* msg);

extern void put_msg_to_async_queue(message_t* msg);

msg_queue_id_t get_queue_id()
{
	return g_msg_queue_id;
}

msg_queue_id_t open_msg_queue(queue_identifier_t queue_identifier,
		int32 max_msg_num)
{

	struct mq_attr attr;
	attr.mq_maxmsg = max_msg_num;
	attr.mq_msgsize = MSG_QUEUE_BUF_SIZE;
	attr.mq_flags = O_RDWR | O_CREAT;
//  mq_unlink(queue_identifier);
	LOG(INFO, "Before set attr.mq_msgsize %ld, attr.mq_maxmsg %ld",
			attr.mq_msgsize, attr.mq_maxmsg);
	mqd_t mq_id = mq_open(queue_identifier, O_RDWR | O_CREAT, FILE_MODE, &attr);
	CHECK(-1 != mq_id, "%s", strerror(errno));
	LOG(INFO, "%s opened as queue %d", queue_identifier, (int)mq_id);
	mq_getattr(mq_id, &attr);
	LOG(INFO, "attr.mq_msgsize %ld, attr.mq_maxmsg %ld",
			attr.mq_msgsize, attr.mq_maxmsg);
	return mq_id;
}

void unlink_msg_queue(queue_identifier_t queue_identifier)
{
	int ret = mq_unlink(queue_identifier);
	CHECK(0 == ret);
}

error_no_t msg_queue_send(msg_queue_id_t msg_queue_id, void* msg_data,
		size_t msg_len, unsigned msg_prio, uint8 retry_times)
{

#if USE_TIMED_SEND
/*Check if we can use mq_timedsend*/
#if (_XOPEN_SOURCE >= 600 || _POSIX_C_SOURCE >= 200112L)
    struct timespec timeout;
	timeout.tv_sec = 0;
	timeout.tv_nsec = MSG_SEND_TIME;

	int ret = mq_timedsend(msg_queue_id,
			(const char *)msg_data,
			msg_len,
			msg_prio,
			&timeout);
	if(0 != ret)
	{
//    LOG(ERROR, "Send msg to msg_queue failed: %s", strerror(errno));
		struct mq_attr attr;
		memset(&attr, 0, sizeof(attr));
		mq_getattr(msg_queue_id, &attr);
		if (attr.mq_curmsgs == attr.mq_maxmsg)
		{
			while(0 != retry_times-- && 0 != ret)
			{
//      usleep(MSG_SEND_TIME/1000);
				ret = mq_timedsend(msg_queue_id,
						(const char *)msg_data,
						msg_len,
						msg_prio,
						&timeout);
			}
		}
		if (0 != ret)
		{
			LOG(ERROR, "Send msg failed attr.mq_curmsgs %ld:%s",
					attr.mq_curmsgs, strerror(errno));
//      exit(0);
			return SEND_MSG_TO_MSG_QUEUE_FAILED_EC;
		}
//    LOG(ERROR, "attr.mq_curmsgs %ld", attr.mq_curmsgs);
	}
	return SUCCESS_EC;
#endif //end  __USE_XOPEN2K
#else
	(void) retry_times;
	return mq_send(msg_queue_id, (const char *) msg_data, msg_len, msg_prio);
#endif
}

ssize_t msg_queue_receive(msg_queue_id_t msg_queue_id, void* msg_data,
		size_t msg_len, unsigned *msg_prio)
{
	return mq_receive(msg_queue_id, (char*) msg_data, msg_len, msg_prio);
}

msg_queue_id_t get_msg_queue_id(uint8 group_id, uint16 app_id,
		int32 max_msg_num)
{
	char queue_name[QUEUE_NAME_LEN] = { 0 };
	snprintf(queue_name, QUEUE_NAME_LEN, "/group-%d_app-%d", group_id, app_id);
	queue_name[QUEUE_NAME_LEN - 1] = '\0';

	LOG(INFO, "Open msg_queue:%s", queue_name);
	msg_queue_id_t msg_queue_id = open_msg_queue(queue_name, max_msg_num);

	return msg_queue_id;
}

msg_queue_id_t get_client_msg_queue_id(uint8 group_id, uint16 app_id)
{
	return get_msg_queue_id(group_id, app_id, CLIENT_MSG_QUEUE_SIZE);
}

msg_queue_id_t get_self_msg_queue_id()
{
	return get_msg_queue_id(get_self_group_id(), get_self_app_id(),
			CLIENT_MSG_QUEUE_SIZE);
}

msg_queue_id_t get_msg_center_queue_id()
{
	return get_msg_queue_id(get_msg_center_group_id(), get_msg_center_app_id(),
			SERVER_MSG_QUEUE_SIZE);
}

void* message_receiver_thread(void* arg)
{
	msg_queue_id_t msg_queue_id = get_self_msg_queue_id();
	CHECK(msg_queue_id != -1);
	(void) arg;

	char msg_buf[MSG_QUEUE_BUF_SIZE] = { 0 };
	unsigned msg_prio = 0;
	while (1)
	{
		memset(msg_buf, 0, MSG_QUEUE_BUF_SIZE);
		ssize_t ret = msg_queue_receive(msg_queue_id, msg_buf,
				MSG_QUEUE_BUF_SIZE, &msg_prio);
		if (ret < 0)
		{
			LOG(ERROR, "receive msg in msg queue failed: %s", strerror(errno));
		}LOG(INFO, "Received a msg form mq");
		msg_header_t mh;
		memset(&mh, 0, sizeof(mh));
		memcpy(&mh, msg_buf, sizeof(mh));
		message_t* msg = allocate_msg_buff(mh.msg_len);
		CHECK(NULL != msg);
		memcpy(msg->buf_head, msg_buf, ret);
//    print_msg_header(msg);
		switch (mh.msg_type)
		{
		case MSG_TYPE_SYNC_RSP:
//      LOG(INFO, "Receive a sync req msg");
			put_rsp_msg_to_sync_queue(msg);
			break;
		case MSG_TYPE_SYNC_REQ:
		case MSG_TYPE_ASYNC_MSG:
			LOG(INFO, "Receive a normal msg");
			put_msg_to_async_queue(msg);
			break;
		default:
			LOG(INFO, "Receive an unknown msg");
			break;
		}
	}

	return NULL;
}

void start_msg_queue(void* thread_data)
{
	g_msg_queue_id = get_msg_center_queue_id();
	create_thread(&g_receiver_thread_id, NULL, message_receiver_thread,
			thread_data);
}

error_no_t send_msg_to_queue(msg_queue_id_t msg_queue_id, message_t* msg,
		uint8 retry_times)
{
	return msg_queue_send(msg_queue_id, (void*) msg->buf_head,
			(size_t) msg->buf_len, msg->header->priority, retry_times);
}

void destroy_msg_queue()
{
	mq_close(g_msg_queue_id);
	cancel_thread(g_receiver_thread_id);
}

