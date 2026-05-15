#include "mts/frame.h"

#include <cstring>
#include <boost/asio/detail/socket_ops.hpp>

namespace mts
{

	FrameHeader parse_header(const char *head4) noexcept
	{
		short msg_id = 0;
		short body_len = 0;
		std::memcpy(&msg_id, head4, HEAD_ID_LEN);
		std::memcpy(&body_len, head4 + HEAD_ID_LEN, HEAD_DATA_LEN);
		msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
		body_len = boost::asio::detail::socket_ops::network_to_host_short(body_len);
		return FrameHeader{msg_id, body_len};
	}

	bool header_is_valid(const FrameHeader &h, std::size_t max_len) noexcept
	{
		return h.body_len >= 0 && static_cast<std::size_t>(h.body_len) <= max_len;
	}

} // namespace mts
