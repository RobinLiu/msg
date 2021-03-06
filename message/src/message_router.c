#include "message_router.h"
#include "message/client/app_info.h"
#include "message/client/app_config.h"
#include "common/base/message.h"
#include "common/base/msg_queue.h"
#include "msg_link.h"

msg_queue_id_t g_app_queue_id[MAX_APP_PER_NODE] = { -1 };

void init_app_queue_id()
{
	uint32 i = 0;
	for (i = 0; i < MAX_APP_PER_NODE; ++i)
	{
		g_app_queue_id[i] = -1;
	}
}

msg_queue_id_t get_app_queue_id(uint8 group_id, uint8 app_id)
{
	/*TODO: find a better way to hash the group id and app it to the index of the queue id
	 * Here I made an assumption that group id on each node is continuous and begin
	 * with the result of an integer multiplies with MAX_GROUP_NUM_IN_NODE.
	 */
	uint32 index = app_id;
	CHECK(index < MAX_APP_PER_NODE);
	if (0 >= g_app_queue_id[index])
	{
		g_app_queue_id[index] = get_client_msg_queue_id(group_id, app_id);
	}
	return g_app_queue_id[index];
}

uint32 num_of_msg = 0;
uint32 num_of_droped_msg = 0;

void process_msg(message_t* msg)
{
	CHECK(NULL != msg);
	node_status_t* node_status;
	switch (msg->header->msg_id)
	{
	case MSG_ID_NODE_STATUS:
		node_status = (node_status_t*) msg->body;
		node_state_changed(node_status->node_id, node_status->node_status);
		break;
	default:
		break;
	}
}

error_no_t router_receive_msg(message_t* msg)
{
//  LOG(ERROR, "Num of msg received is %d", );
//  LOG(INFO, "New package received from other node, route it to app");
//  print_msg_header(msg);
	if (msg->header->rcver.group_id == get_self_group_id()
			&& msg->header->rcver.app_id == get_self_app_id())
	{
		//this is a message to myself.
		LOG(INFO, "Control message for router is received...");
		process_msg(msg);
		free_msg_buff(&msg);
		return SUCCESS_EC;
	}
	msg_queue_id_t msg_queue_id = get_app_queue_id(msg->header->rcver.group_id,
			msg->header->rcver.app_id);

	if (0 != send_msg_to_queue(msg_queue_id, msg, RETRY_TIMES))
	{
		++num_of_droped_msg;
	}
	free_msg_buff(&msg);
//  LOG(ERROR, "received %d, droped %d", ++num_of_msg, num_of_droped_msg);
//  if (100000 == num_of_msg) {
//    exit(0);
//  }
	return SUCCESS_EC;
}

error_no_t send_msg_by_node(message_t* msg, node_id_t dest_node_id,
		node_id_t self_node_id)
{
	if (dest_node_id == self_node_id)
	{
		return router_receive_msg(msg);
	}
	else
	{
		return send_msg_to_node(msg, dest_node_id);
	}
}

error_no_t send_msg_to_msg_router(message_t* msg)
{
	if (NULL == msg)
	{
		LOG(WARNING, "Pass NULL pointer to function.");
		return NULL_POINTER_EC;
	}
	node_id_t self_node = get_self_node_id();
	if (self_node == NODE_NOT_EXIST)
	{
		LOG(WARNING, "Get self node id failed");
		free_msg_buff(&msg);
		return NODE_NOT_EXIST_EC;
	}

	node_id_t node_list[2];
	uint8 node_num = 0;
	node_num = get_app_location(msg->header->rcver, node_list);
	CHECK(node_num <= 2);
	//TODO: Check the destination state, if the destination is not reachable,
	//ignore the message.

	error_no_t ret1, ret2;
	switch (node_num)
	{
	case 0:
		LOG(WARNING, "Get app location failed");
		return NODE_NOT_EXIST_EC;
		break;
	case 1:
		return send_msg_by_node(msg, node_list[0], self_node);
		break;
	case 2:
		ret1 = send_msg_by_node(msg, node_list[0], self_node);
		//TODO: message should be cloned.
		ret2 = send_msg_by_node(msg, node_list[1], self_node);
		if (ret1 == SUCCESS_EC && ret2 == SUCCESS_EC)
		{
			return SUCCESS_EC;
		}
		else
		{
			//return only one error code;
			return (ret1 == SUCCESS_EC ? ret2 : ret1);
		}
		break;
	default:
		return CODE_NEVER_EXCUTED_EC;
		break;
	}
}
