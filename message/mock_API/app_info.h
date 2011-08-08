#ifndef APP_MGR_APP_INFO_H
#define APP_MGR_APP_INFO_H
#include "common/include/basic_types.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NODE_NUM                  16
#define MAX_GROUP_NUM_IN_NODE         16
#define MAX_APP_NUM_IN_GROUP          64

#define NODE_NOT_EXIST                0xffff

uint8 get_self_group_id();
uint16 get_self_app_id();
uint8 get_self_role();

uint8 get_app_location(msg_receiver_t app, node_id_t* node_list);
node_id_t get_self_node_id();

void init_msg_center();

void get_self_mac(uint8* mac);
void get_peer_mac(node_id_t node_id, uint8* mac);

uint8 get_msg_center_group_id();
uint16 get_msg_center_app_id();
void get_serv_info(node_id_t node_id, int8* serv_ip, uint16* serv_port);
uint16 get_self_port();
#ifdef __cplusplus
}
#endif
#endif
