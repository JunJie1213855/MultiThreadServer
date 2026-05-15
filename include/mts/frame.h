#ifndef MTS_FRAME_H_
#define MTS_FRAME_H_

#include <cstddef>

namespace mts
{

	// 线协议帧格式：[msg_id:2][body_len:2][body:N]，msg_id 与 body_len 为网络字节序。
	// 这是整个库唯一的帧编解码接口（the framing seam）：
	//   - 解码（net->host）在此 parse_header / header_is_valid；
	//   - 编码（host->net）在 msg_node.cpp 的 SendNode 构造里，属于同一套协议。
	inline constexpr int HEAD_TOTAL_LEN = 4;
	inline constexpr int HEAD_ID_LEN = 2;
	inline constexpr int HEAD_DATA_LEN = 2;

	struct FrameHeader
	{
		short msg_id;
		short body_len;
	};

	// 从 4 字节包头解码（网络序 -> 主机序）。
	FrameHeader parse_header(const char *head4) noexcept;

	// 校验包体长度合法（非负且不超过 max_len）。
	bool header_is_valid(const FrameHeader &h, std::size_t max_len) noexcept;

} // namespace mts

#endif
