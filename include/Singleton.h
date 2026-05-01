#ifndef SINGLETON_H_
#define SINGLETON_H_

#include <memory>
#include <mutex>
#include <iostream>
using namespace std;

template <typename T>
class Singleton
{
protected:
	Singleton() = default;

	// 禁止拷贝
	Singleton(const Singleton<T> &) = delete;
	Singleton &operator=(const Singleton<T> &st) = delete;

	// 禁止移动
	Singleton(Singleton<T> &&) = delete;
	Singleton &operator=(Singleton<T> &&st) = delete;
	~Singleton() = default;

public:
	static std::shared_ptr<T> GetInstance()
	{
		static std::once_flag s_flag;
		std::call_once(s_flag, [&]()
					   { _instance = shared_ptr<T>(new T); });

		return _instance;
	}

protected:
	static std::shared_ptr<T> _instance;
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif
