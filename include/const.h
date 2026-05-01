#ifndef CONST_H_
#define CONST_H_

// #define MAX_LENGTH 1024 * 2
// #define HEAD_TOTAL_LEN 4
// #define HEAD_ID_LEN 2
// #define HEAD_DATA_LEN 2
// #define MAX_RECVQUE 10000
// #define MAX_SENDQUE 1000

constexpr int MAX_LENGTH = 1024 * 2;
constexpr int HEAD_TOTAL_LEN = 4;
constexpr int HEAD_ID_LEN = 2;
constexpr int HEAD_DATA_LEN = 2;
constexpr int MAX_RECVQUE = 10000;
constexpr int MAX_SENDQUE = 1000;


enum MSG_IDS
{
	MSG_ID_MIN = 1000,
	MSG_HELLO_WORD = 1001,
	MSG_ID_MAX
};

#endif