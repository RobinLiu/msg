#include "message/client/app_config.h"
#include "message/client/msg_client.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/client/app_info.h"

#define MSG_ID_TEST_MSG 1

void print_usage(char** argv)
{
    printf("Usage: %s [self group id] [self app id] [dst group id] [dst app id] \n",
            argv[0]);
}

int main(int argc, char** argv)
{
    if(argc != 5)
    {
        print_usage(argv);
        exit(0);
    }
    uint8 self_grp_id = (uint8)atoi(argv[1]);
    uint16 self_app_id = (uint16)atoi(argv[2]);
    uint8 dst_grp_id = (uint8)atoi(argv[3]);
    uint16 dst_app_id = (uint16)atoi(argv[4]);

    if(self_grp_id >= MAX_GROUP_NUM
       || self_app_id >= MAX_GROUP_NUM || self_app_id == 0
       || dst_grp_id >= MAX_APP_PER_NODE
       || dst_app_id >= MAX_APP_PER_NODE || dst_app_id == 0)
    {
        printf("Group id is betwwen [0 - %d) and "
               "APP id is betwwen [1 - %d) \n",
               MAX_GROUP_NUM, MAX_APP_PER_NODE);
        exit(0);
    }

    init_msg_system(self_grp_id, self_app_id);

    long long received_msg = 0;
    message_t* msg = NULL;
    int ret = 1;
    while (1)
    {
        receive_msg(&msg);
        uint32 msg_id = msg->header->msg_id;
        //LOG(INFO, "Received a msg in app id is %d, response with ack", msg->header->msg_id);
        //print_msg_header(msg);
        switch (msg_id)
        {
        case MSG_ID_TEST_MSG:
            ret++;
            LOG(INFO, "Begin to send rsp msg...");
//      msg->header->msg_seq = 0;
            send_sync_msg_rsp(msg, &ret, sizeof(ret));
            received_msg++;
            break;
        default:
            received_msg++;
            //LOG(IMPORTANT, "Message id %d is received but not processed", msg_id);
            //print_msg_header(msg);
            break;
        }
        if(received_msg % 1000 == 0)
        {
            LOG(IMPORTANT, "Received %ld messages", received_msg);
        }
        free_msg_buff(&msg);
    }
    return 0;
}
