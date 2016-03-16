// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2016, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "exception.hpp"
#include "log.hpp"

namespace Poseidon {

Exception::Exception(const char *file, std::size_t line, const char *func, SharedNts message)
	: m_file(file), m_line(line), m_func(func), m_message(STD_MOVE(message))
{
	LOG_POSEIDON(Logger::SP_MAJOR | Logger::LV_INFO,
		"Constructing Exception: file = ", m_file, ", line = ", m_line, ", func = ", m_func, ", message = ", m_message);
}
Exception::~Exception() NOEXCEPT {
}

}
