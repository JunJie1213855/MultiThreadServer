#include "mts/msg_node.h"

#include <cstring>
#include <boost/asio/detail/socket_ops.hpp>

namespace mts
{

	// 接收消息节点构造函数
	RecvNode::RecvNode(short max_len, short msg_id)
		: MsgNode(max_len), _msg_id(msg_id)
	{
	}

	// 发送消息节点构造函数。
	// 消息格式：[2字节 msg_id][2字节 长度][数据] —— 即 frame.h 描述的同一套帧协议，
	// 此处是它的“编码端”（host->net），解码端在 frame.cpp。
	SendNode::SendNode(const char *msg, short max_len, short msg_id)
		: MsgNode(max_len + HEAD_TOTAL_LEN), _msg_id(msg_id), _retry_count(0)
	{
		// 将 msg_id 转为网络字节序
		short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
		std::memcpy(_data, &msg_id_net, HEAD_ID_LEN);
		// 将长度转为网络字节序
		short body_len_net = boost::asio::detail::socket_ops::host_to_network_short(max_len);
		std::memcpy(_data + HEAD_ID_LEN, &body_len_net, HEAD_DATA_LEN);
		// 拷贝数据
		std::memcpy(_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
	}

} // namespace mts
