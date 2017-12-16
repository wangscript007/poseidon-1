// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2017, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "promise.hpp"
#include "exception.hpp"
#include "log.hpp"
#include "singletons/job_dispatcher.hpp"

namespace Poseidon {

Promise::Promise() NOEXCEPT
	: m_satisfied(false), m_except()
{ }
Promise::~Promise(){
	if(!m_satisfied){
		LOG_POSEIDON_WARNING("Destroying an unsatisfied Promise.");
	}
}

bool Promise::would_throw() const NOEXCEPT {
	const RecursiveMutex::UniqueLock lock(m_mutex);
	if(!m_satisfied){
		return true;
	}
	if(m_except){
		return true;
	}
	return false;
}
void Promise::check_and_rethrow() const {
	const RecursiveMutex::UniqueLock lock(m_mutex);
	DEBUG_THROW_UNLESS(m_satisfied, Exception, sslit("Promise has not been satisfied"));
	if(m_except){
		STD_RETHROW_EXCEPTION(m_except);
	}
}

void Promise::set_success(){
	const RecursiveMutex::UniqueLock lock(m_mutex);
	DEBUG_THROW_UNLESS(!m_satisfied, Exception, sslit("Promise has already been satisfied"));
	m_satisfied = true;
	m_except = VAL_INIT;
}
void Promise::set_exception(STD_EXCEPTION_PTR except){
	const RecursiveMutex::UniqueLock lock(m_mutex);
	DEBUG_THROW_UNLESS(!m_satisfied, Exception, sslit("Promise has already been satisfied"));
	m_satisfied = true;
	m_except = STD_MOVE_IDN(except);
}

void yield(const boost::shared_ptr<const Promise> &promise, bool insignificant){
	JobDispatcher::yield(promise, insignificant);
}

}
