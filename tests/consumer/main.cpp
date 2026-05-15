// 冒烟测试：证明导出的头文件能编译、库能链接、符号能解析。
// 只调用一个真正在 libmts 里实现的符号（mts::parse_header），不产生任何副作用
// （不绑端口、不起线程）。
#include "mts/mts.h"
#include <iostream>

int main()
{
	char head[mts::HEAD_TOTAL_LEN] = {};
	mts::FrameHeader h = mts::parse_header(head);
	bool ok = mts::header_is_valid(h, 2048);

	mts::ServerConfig cfg;
	cfg.port = 0;

	std::cout << "mts consumer smoke: linked OK"
			  << " (msg_id=" << h.msg_id
			  << ", valid=" << ok
			  << ", default max_msg_length=" << cfg.max_msg_length << ")\n";
	return 0;
}
