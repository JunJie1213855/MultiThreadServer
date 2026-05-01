#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <json/json.h>
#include <chrono>
#include <mutex>

using namespace boost::asio::ip;

const int MAX_LENGTH = 1024 * 2;
const int HEAD_LENGTH = 2;
const int HEAD_TOTAL = 4;

std::mutex cout_mutex;

// 每个线程独立的客户端逻辑
void client_thread(int thread_id) {
    try {
        // std::this_thread::sleep_for(std::chrono::seconds(1));

        // 每个线程有自己的 ioc，完全独立，无竞争
        boost::asio::io_context ioc;
        tcp::socket sock(ioc);

        tcp::endpoint remote_ep(
            boost::asio::ip::address::from_string("127.0.0.1"), 10086);

        boost::system::error_code error;
        sock.connect(remote_ep, error);
        if (error) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "connect failed: " << error.message() << "\n";
            return;  // void 函数，直接 return
        }

        Json::Value root;
        Json::Reader reader;

        for (int i = 0; i < 500; i++) {
            // 构造消息
            root["id"]   = 1001;
            root["data"] = "hello world";
            std::string request = root.toStyledString();
            size_t request_length = request.length();

            // 构造包头：[msgid:2][len:2][body:N]
            char send_data[MAX_LENGTH] = {0};
            short msgid      = boost::asio::detail::socket_ops::host_to_network_short(1001);
            short body_len   = boost::asio::detail::socket_ops::host_to_network_short(
                                   static_cast<short>(request_length));
            memcpy(send_data,     &msgid,    2);
            memcpy(send_data + 2, &body_len, 2);
            memcpy(send_data + 4, request.c_str(), request_length);

            boost::asio::write(sock,
                boost::asio::buffer(send_data, request_length + HEAD_TOTAL));

            // 读包头
            char reply_head[HEAD_TOTAL] = {0};
            size_t n = boost::asio::read(sock,
                boost::asio::buffer(reply_head, HEAD_TOTAL));
            if (n != HEAD_TOTAL) break;

            short recv_msgid = 0, recv_len = 0;
            memcpy(&recv_msgid, reply_head,     HEAD_LENGTH);
            memcpy(&recv_len,   reply_head + 2, HEAD_LENGTH);
            recv_msgid = boost::asio::detail::socket_ops::network_to_host_short(recv_msgid);
            recv_len   = boost::asio::detail::socket_ops::network_to_host_short(recv_len);

            if (recv_len <= 0 || recv_len > MAX_LENGTH) break;

            // 读包体
            char msg[MAX_LENGTH] = {0};
            size_t msg_len = boost::asio::read(sock,
                boost::asio::buffer(msg, recv_len));
            if (msg_len != static_cast<size_t>(recv_len)) break;

            reader.parse(std::string(msg, msg_len), root);
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "[thread " << thread_id << "] i=" << i
                          << " id=" << root["id"]
                          << " data=" << root["data"] << "\n";
            }
        }
    } catch (std::exception &e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < 100; i++) {
        threads.emplace_back(client_thread, i);
    }

    for (auto &t : threads) t.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time: "
              << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
              << "s\n";
    return 0;
}