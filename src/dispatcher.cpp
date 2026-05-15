#include "mts/dispatcher.h"
#include "mts/session.h"
#include "mts/msg_node.h"

namespace mts
{

	void Dispatcher::on(short msg_id, Handler h)
	{
		handlers_[msg_id] = std::move(h);
	}

	// 把消息投递到 session 对应的 io_context 上处理
	void Dispatcher::dispatch(std::shared_ptr<Session> session,
							  std::shared_ptr<RecvNode> recvnode)
	{
		boost::asio::post(session->GetSocket().get_executor(),
						  [this, session, recvnode]
						  {
							  auto it = handlers_.find(recvnode->_msg_id);
							  if (it == handlers_.end())
								  return;
							  it->second(session, recvnode->_msg_id,
										 std::string(recvnode->_data, recvnode->_cur_len));
						  });
	}

} // namespace mts
