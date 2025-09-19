// #ifndef __SDK_UART_SEND_RESPONSE_CFG_H__
// #define __SDK_UART_SEND_RESPONSE_CFG_H__


// typedef enum {
//     RESP_STATE_IDLE,          //帧头1
//     RESP_STATE_HEADER_BYTE2,  //帧头2
//     RESP_STATE_EVENT_CODE,    //事件码
//     RESP_STATE_DATA_LENGTH,   //数据长度
//     RESP_STATE_DATA,          //数据内容
//     RESP_STATE_CHECKSUM,      //校验和
//     RESP_STATE_FOOTER         //帧尾
// }receiving_messaging_protocol;
// typedef struct {
//     uint8_t frame_header[2];   // 帧头 2字节
//     uint8_t event_code[2];     // 事件码 2字节
//     uint8_t data_length[2];    // 数据长度 2字节
//     uint8_t *data;             // 数据
//     uint8_t checksum[2];       // 校验和 2字节
//     uint8_t frame_footer;   // 帧尾 1字节
// }qtk_uart_recv_frame_t;
// typedef enum {
//     PARSE_STATE_HEADER1,
//     PARSE_STATE_HEADER2,
//     PARSE_STATE_EVENT_CODE,
//     PARSE_STATE_DATA_LEN,
//     PARSE_STATE_DATA,
//     PARSE_STATE_CRC,
//     PARSE_STATE_FOOTER
// } uart_parse_state_t;

// void send_version_response(uc, frame);
// #endif