#ifndef BASE_MSG_CLIENT_H
#define BASE_MSG_CLIENT_H
#include "common/include/basic_types.h"

#define MSG_ID(msg)    (msg->header->msg_id)

/*****************************************************************************
 * Application first call this function to initialize the message system.
 * No return value is provided, if this call failed, the applicaiton exit
 * immediately, since it application failed to initialized wouldn't work as
 * expected.
 ****************************************************************************/
void init_msg_system();


/*****************************************************************************
 * Application use this interface to send asynchronized message.
 * Input: @dst_grp_id message destination group id
 *        @dst_app_id message destination application id
 *        @msg_id     message id
 *        @msg_priority message priority
 *        @msg_content the buffer which contains the content need to be sent
 *        @msg_len    the length of the message.
 * Return: SUCCESS_EC on success, other value for failure.
 ****************************************************************************/
error_no_t send_msg(uint8  dst_grp_id,
                    uint16 dst_app_id,
                    uint32 msg_id,
                    uint8  msg_priority,
                    void*  msg_content,
                    uint32 msg_len);


/***************************************************************uo **************
 * Interface Used to receive message from other application.
 * Note, this interface will not return if there's no message from other
 * application, and will return immediately if there's a asynchronized message.
 * Input: @msg, a double pointer to the received msg. once the function returns,
 *              there this pointer will point to the pointer of one received
 *              message in the message queue. user don't need to allocate memory
 *              for the received message, just provide the pointer.
 *              Note: User need to call void free_msg_buff(message_t** msg); to
 *              release the message buffer.
 * Return: No return value
 ****************************************************************************/

void receive_msg(message_t** msg);

/*****************************************************************************
 * Application use this interface to send synchronized message. The call to
 * this function will be blocked if there's no response message received in the
 * given time_out time.
 * Input: @dst_grp_id message destination group id
 *        @dst_app_id message destination application id
 *        @msg_id     message id
 *        @msg_priority message priority
 *        @msg_content the buffer which contains the content need to be sent
 *        @msg_len    the length of the message.
 *        @time_out   timeout time in milliseconds
 *        @rsp_buf    buffer allocated by user which used to store the
 *                    response from remote.
 *        @rsp_buf_size the length of the rsp_buf
 * Return: SUCCESS_EC on success, other value for failure.
 ****************************************************************************/

error_no_t send_sync_msg(uint8  dst_grp_id,
                    uint16 dst_app_id,
                    uint32 msg_id,
                    uint8  msg_priority,
                    void*  msg_content,
                    uint32 msg_len,
                    uint32 time_out,
                    void*  rsp_buf,
                    uint32 rsp_buf_size);


/*****************************************************************************
 * Application use this interface to send response for the received synchronized
 * message.
 * Input: @syn_req_msg the received synchronized message
 *        @rsp_buf    buffer which contains the response for the received message.
 *        @rsp_buf_len the length of the rsp_buf
 * Return: SUCCESS_EC on success, other value for failure.
 ****************************************************************************/
error_no_t send_sync_msg_rsp(message_t* syn_req_msg,
                    void* rsp_buf,
                    int rsp_buf_size);



#endif
