#include "MsgNode.h"

// 接收消息节点构造函数
RecvNode::RecvNode(short max_len, short msg_id) : MsgNode(max_len),
												  _msg_id(msg_id)
{
}

// 发送消息节点构造函数
// 消息格式：[2字节 msg_id][2字节 长度][数据]
SendNode::SendNode(const char *msg, short max_len, short msg_id) : MsgNode(max_len + HEAD_TOTAL_LEN), _msg_id(msg_id), _retry_count(0)
{
	// 将 msg_id 转为网络字节序
	short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	memcpy(_data, &msg_id_host, HEAD_ID_LEN);
	// 将长度转为网络字节序
	short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	memcpy(_data + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);
	// 拷贝数据
	memcpy(_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
}