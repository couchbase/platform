/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

/**
 * This is a wrapping layer on top of BSD sockets to hide away the difference
 * between Windows and Linux (the first use char pointers and the latter
 * use void etc). This allows us to hide all of the platform difference in
 * one place and not have to do #ifdef's all over the place
 */
#pragma once

#include <platform/platform.h>
#include <platform/platform_socket.h>
#include <platform/socket-visibility.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int in_port_t;
typedef int sa_family_t;
#define SOCKETPAIR_AF AF_INET
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH

#else
#include <netinet/in.h> // For in_port_t
#include <sys/socket.h>
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SOCKETPAIR_AF AF_UNIX
#endif

#include <cerrno>
#include <cstdint>
#include <string>

namespace cb {
namespace net {

CBSOCKET_PUBLIC_API
int closesocket(SOCKET s);

CBSOCKET_PUBLIC_API
int get_socket_error();

CBSOCKET_PUBLIC_API
int bind(SOCKET sock, const struct sockaddr* name, socklen_t namelen);

CBSOCKET_PUBLIC_API
SOCKET accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen);

CBSOCKET_PUBLIC_API
int connect(SOCKET sock, const struct sockaddr* name, size_t namelen);

CBSOCKET_PUBLIC_API
SOCKET socket(int domain, int type, int protocol);

CBSOCKET_PUBLIC_API
int shutdown(SOCKET sock, int how);

CBSOCKET_PUBLIC_API
ssize_t send(SOCKET sock, const void* buffer, size_t length, int flags);

CBSOCKET_PUBLIC_API
ssize_t sendmsg(SOCKET sock, const struct msghdr* message, int flags);

CBSOCKET_PUBLIC_API
ssize_t sendto(SOCKET sock,
               const void* buffer,
               size_t length,
               int flags,
               const struct sockaddr* dest_addr,
               socklen_t dest_len);

CBSOCKET_PUBLIC_API
ssize_t recv(SOCKET sock, void* buffer, size_t length, int flags);

CBSOCKET_PUBLIC_API
ssize_t recvfrom(SOCKET sock,
                 void* buffer,
                 size_t length,
                 int flags,
                 struct sockaddr* address,
                 socklen_t* address_len);

CBSOCKET_PUBLIC_API
ssize_t recvmsg(SOCKET sock, struct msghdr* message, int flags);

CBSOCKET_PUBLIC_API
int getsockopt(SOCKET sock,
               int level,
               int option_name,
               void* option_value,
               socklen_t* option_len);

CBSOCKET_PUBLIC_API
int setsockopt(SOCKET sock,
               int level,
               int option_name,
               const void* option_value,
               socklen_t option_len);

CBSOCKET_PUBLIC_API
int socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]);

CBSOCKET_PUBLIC_API
int set_socket_noblocking(SOCKET sock);

CBSOCKET_PUBLIC_API
int listen(SOCKET sock, int backlog);


#ifdef WIN32
inline int is_blocking(DWORD dw = get_socket_error()) {
    return (dw == WSAEWOULDBLOCK);
}
inline int is_interrupted(DWORD dw = get_socket_error()) {
    return (dw == WSAEINTR);
}
inline int is_emfile(DWORD dw = get_socket_error()) {
    return (dw == WSAEMFILE);
}
inline int is_closed_conn(DWORD dw = get_socket_error()) {
    return (dw == WSAENOTCONN || dw == WSAECONNRESET);
}
inline int is_addrinuse(DWORD dw = get_socket_error()) {
    return (dw == WSAEADDRINUSE);
}
inline void set_ewouldblock(void) {
    WSASetLastError(WSAEWOULDBLOCK);
}
inline void set_econnreset(void) {
    WSASetLastError(WSAECONNRESET);
}
#else
inline int is_blocking(int dw = get_socket_error()) {
    return (dw == EAGAIN || dw == EWOULDBLOCK);
}
inline int is_interrupted(int dw = get_socket_error()) {
    return (dw == EINTR || dw == EAGAIN);
}
inline int is_emfile(int dw = get_socket_error()) {
    return (dw == EMFILE);
}
inline int is_closed_conn(int dw = get_socket_error()) {
    return  (dw == ENOTCONN || dw != ECONNRESET);
}
inline int is_addrinuse(int dw = get_socket_error()) {
    return (dw == EADDRINUSE);
}
inline void set_ewouldblock(void) {
    errno = EWOULDBLOCK;
}
inline void set_econnreset(void) {
    errno = ECONNRESET;
}
#endif

/**
 * Get a textual representation of the address represented in sockaddr_storage
 */
CBSOCKET_PUBLIC_API
std::string to_string(const struct sockaddr_storage* addr, socklen_t addr_len);

/**
 * Get the name of the socket in a textual form like:
 *
 *    127.0.0.1:11211
 *    [::1]:11211
 *
 * @param sfd The socket to get the name for
 * @throws std::exception if one of the functions needed to look up the socket
 *                        name fails
 */
CBSOCKET_PUBLIC_API
std::string getsockname(SOCKET sfd);

/**
 * Get the name of the peer in a textual form like:
 *
 *    127.0.0.1:11211
 *    [::1]:11211
 *
 * @param sfd The socket to get the name for
 * @throws std::exception if one of the functions needed to look up the peer
 *                        name fails
 */
CBSOCKET_PUBLIC_API
std::string getpeername(SOCKET sfd);

} // namespace net
} // namespace cb
