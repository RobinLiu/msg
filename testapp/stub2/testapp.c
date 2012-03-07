#include "message/client/app_config.h"
#include "message/client/msg_client.h"
#include "common/base/message.h"
#include "common/include/common.h"
#include "message/client/app_info.h"

void print_usage(char** argv)
{
    printf("Usage: %s [self group id] [self app id] [dst group id] [dst app id] \n",
            argv[0]);
}

int main(int argc, char** argv)
{
    if (argc != 5)
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

    int msg_content = 0;
    int rsp = 0;
    int8 msg_priority = 1;
    uint32 time_out = 10000;
    uint32 msg_id = 1;
    long long msg_send_num = 0;
    while (1)
    {
        int ret = send_sync_msg(dst_grp_id, dst_app_id, msg_id, msg_priority,
                &msg_content, sizeof(msg_content), time_out, &rsp, sizeof(rsp));
        msg_content++;
        msg_send_num++;
        //LOG(IMPORTANT, "sync msg result is %d, and rsp is %d", ret, rsp);
        ret = send_msg(dst_grp_id, dst_app_id, 2, msg_priority, &msg_content,
                sizeof(msg_content));
        msg_send_num++;
        if(msg_send_num % 1000 == 0)
        {
            LOG(IMPORTANT, "Number of sent msg is %ld", msg_send_num);
        }
    }
    return 0;
}
