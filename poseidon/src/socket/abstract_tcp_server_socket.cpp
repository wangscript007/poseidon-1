// This file is part of Poseidon.
// Copyleft 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_tcp_server_socket.hpp"
#include "abstract_tcp_socket.hpp"
#include "../utils.hpp"

namespace poseidon {

Abstract_TCP_Server_Socket::
Abstract_TCP_Server_Socket(const Socket_Address& addr)
  : Abstract_Accept_Socket(addr.family())
  {
    this->do_socket_listen(addr);
  }

Abstract_TCP_Server_Socket::
~Abstract_TCP_Server_Socket()
  {
  }

uptr<Abstract_Socket>
Abstract_TCP_Server_Socket::
do_socket_on_accept(unique_FD&& fd, const Socket_Address& addr)
  {
    return this->do_socket_on_accept_tcp(::std::move(fd), addr);
  }

void
Abstract_TCP_Server_Socket::
do_socket_on_register(rcptr<Abstract_Socket>&& sock)
  {
    return this->do_socket_on_register_tcp(
        ::rocket::static_pointer_cast<Abstract_TCP_Socket>(
                    ::std::move(sock)));
  }

}  // namespace poseidon
