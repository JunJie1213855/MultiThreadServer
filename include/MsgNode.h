#ifndef MSGNODE_H_
#define MSGNODE_H_

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include "const.h"
using boost::asio::ip::tcp;

namespace mts { class Dispatcher; }

class MsgNode
{
public:
	MsgNode(short max_len) : _total_len(max_len), _cur_len(0)
	{
		_data = new char[_total_len + 1]();
		_data[_total_len] = '\0';
	}

	~MsgNode()
	{
		delete[] _data;
	}

	MsgNode(const MsgNode &) = delete;
	MsgNode &operator=(const MsgNode &) = delete;

	void Clear()
	{
		::memset(_data, 0, _total_len);
		_cur_len = 0;
	}

	short _cur_len;
	short _total_len;
	char *_data;
};

class RecvNode : public MsgNode
{
	friend class mts::Dispatcher;

public:
	RecvNode(short max_len, short msg_id);

private:
	short _msg_id;
};

class SendNode : public MsgNode
{
	friend class mts::Dispatcher;
	friend class CSession;

public:
	SendNode(const char *msg, short max_len, short msg_id);

private:
	short _msg_id;
	int _retry_count;
};

#endif
