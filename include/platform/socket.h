/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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
#pragma once

/**
 * This is a wrapping layer on top of BSD sockets to allow for logging all
 * of the data being transferred. It was initially created in order to
 * track down why one of the pipes in moxi seemed to get out of sync
 * protocol wise, but may be useful in the future. This extra layer cause
 * an extra lookup of an atomic variable to determine if logging should
 * be performed or not which shouldn't affect the performance too much.
 *
 * As it was initially created for locating a bug in moxi, a C API exists
 * with a 'cb_' prefix. It all wraps into methods in cb::net namespace.
 *
 * Each method works as documented in the in the OS.
 *
 * One could use tcpdump to collect the same info, but I failed to get
 * tcpdump create a pcap file I know how to parse when using the loopback
 * interface. Whipping up this seemed a lot easier ;-)
 *
 * When using the logging feature of cbsocket a file is created in
 * /tmp by using the following naming convention:
 *
 *     pid-socketid-seqno
 *
 * Seqno is needed as the socket identifiers may be reused by the operating
 * system. Each file consists data received and sent over the socket, by
 * using a header followed by the actual data. The header looks like:
 *
 *
 *     Byte/     0       |       1       |       2       |       3       |
 *        /              |               |               |               |
 *       |0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
 *       +---------------+---------------+---------------+---------------+
 *      0|                                                               |
 *       +                64 bit steady-clock offset in 𝝁s               +
 *      4|                                                               |
 *       +---------------+---------------+---------------+---------------+
 *      8|               32 bit number of bytes in payload               |
 *       +---------------+---------------+---------------+---------------+
 *     12| direction r/w |
 *       +---------------+
 *       Total 13 bytes
 *
 * The direction byte contains either 'r' or 'w' where an 'r' indicates
 * that the data was read off the socket, and 'w' means it was sent over
 * the socket.
 *
 */

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
#include <sys/socket.h>
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SOCKETPAIR_AF AF_UNIX
#endif

#include <platform/platform.h>
#include <platform/socket-visibility.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#include <platform/sized_buffer.h>

#include <cerrno>
#include <functional>
#include <string>

extern "C" {
#endif

CBSOCKET_PUBLIC_API
int cb_closesocket(SOCKET s);

CBSOCKET_PUBLIC_API
int cb_get_socket_error();

CBSOCKET_PUBLIC_API
int cb_bind(SOCKET sock, const struct sockaddr* name, socklen_t namelen);

CBSOCKET_PUBLIC_API
SOCKET cb_accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen);

CBSOCKET_PUBLIC_API
int cb_connect(SOCKET sock, const struct sockaddr* name, int namelen);

CBSOCKET_PUBLIC_API
SOCKET cb_socket(int domain, int type, int protocol);

CBSOCKET_PUBLIC_API
int cb_shutdown(SOCKET sock, int how);

CBSOCKET_PUBLIC_API
ssize_t cb_send(SOCKET sock, const void* buffer, size_t length, int flags);

CBSOCKET_PUBLIC_API
ssize_t cb_sendmsg(SOCKET sock, const struct msghdr* message, int flags);

CBSOCKET_PUBLIC_API
ssize_t cb_sendto(SOCKET sock,
                  const void* buffer,
                  size_t length,
                  int flags,
                  const struct sockaddr* dest_addr,
                  socklen_t dest_len);

CBSOCKET_PUBLIC_API
ssize_t cb_recv(SOCKET sock, void* buffer, size_t length, int flags);

CBSOCKET_PUBLIC_API
ssize_t cb_recvfrom(SOCKET sock,
                    void* buffer,
                    size_t length,
                    int flags,
                    struct sockaddr* address,
                    socklen_t* address_len);

CBSOCKET_PUBLIC_API
ssize_t cb_recvmsg(SOCKET sock, struct msghdr* message, int flags);

CBSOCKET_PUBLIC_API
int cb_socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]);

CBSOCKET_PUBLIC_API
int cb_set_socket_noblocking(SOCKET sock);

CBSOCKET_PUBLIC_API
void cb_set_socket_logging(bool enable);

CBSOCKET_PUBLIC_API
int cb_getsockopt(SOCKET sock,
                  int level,
                  int option_name,
                  void* option_value,
                  socklen_t* option_len);

CBSOCKET_PUBLIC_API
int cb_setsockopt(SOCKET sock,
                  int level,
                  int option_name,
                  const void* option_value,
                  socklen_t option_len);

CBSOCKET_PUBLIC_API
int cb_listen(SOCKET sock, int backlog);

#ifdef __cplusplus
}

namespace cb {
namespace net {

/**
 * Specify a callback method to use to allow the caller to filter out
 * _some_ connections instead of logging all of them.
 *
 * You might for instance just care about traffic to a given port, which
 * may be implemented with something like:
 *
 *     static bool filterPeerPort(SOCKET sock) {
 *         struct sockaddr_storage peer{};
 *         socklen_t peer_len = sizeof(peer);
 *         int err;
 *         if ((err = getpeername(sock,
 *                                reinterpret_cast<struct sockaddr*>(&peer),
 *                                &peer_len)) != 0) {
 *             return false;
 *         }
 *
 *         char host[50];
 *         char port[50];
 *
 *         err = getnameinfo(reinterpret_cast<struct sockaddr*>(&peer),
 *                           peer_len,
 *                           host, sizeof(host),
 *                           port, sizeof(port),
 *                           NI_NUMERICHOST | NI_NUMERICSERV);
 *         if (err != 0) {
 *             return false;
 *         }
 *
 *         return (atoi(port) == 11210);
 *     }
 *
 * The callback function should return true in order to enable logging for
 * the given socket, or false to ignore the socket. Note that the global
 * logging flag must be set in order to enable logging (and this filter).
 *
 * @param callback The method to use, or nullptr to reset to the default.
 */
CBSOCKET_PUBLIC_API
void set_log_filter_handler(bool (* callback)(SOCKET));

/**
 * Specify a callback method to be called when a given socket is closed.
 * The default on_close_handler removes the log file.
 *
 * @param callback The new handler to install, or nullptr to reset to the
 *                 default handler.
 */
CBSOCKET_PUBLIC_API
void set_on_close_handler(void (*callback)(SOCKET, const std::string& file));

/**
 * Enable / disable socket logging.
 *
 * When enabled each socket creates a file in /tmp by using the
 * following name `/tmp/<pid>-<socketfd>-seqno`.
 *
 * @param enable set to true to start logging
 */
CBSOCKET_PUBLIC_API
void set_socket_logging(bool enable);

enum class Direction : uint8_t { Send, Receive };

/**
 * The iterator function to be called for each packet in the dump file
 *
 * @param timestamp the offset in 𝝁s
 * @param direction if the data was sent or received
 * @param data the actual payload in the packet
 * @return true to continue parsing, false otherwise
 */
using IteratorFunc = std::function<bool(
        uint64_t timestamp, Direction direction, cb::const_byte_buffer data)>;

/**
 * Iterate over a socket log file
 *
 * @param file the file to iterate over
 * @param callback The callback function to call for each packet in
 *                 the logfile
 * @throws std::system_error if there is a problem accessing the file
 * @throws std::bad_alloc for memory allocation issues
 * @throws std::invalid_argument if the file contains an illegal format
 */
CBSOCKET_PUBLIC_API
void iterate_logfile(const std::string& file, IteratorFunc callback);

CBSOCKET_PUBLIC_API
void iterate_logfile(cb::const_byte_buffer buffer, IteratorFunc callback);

CBSOCKET_PUBLIC_API
int closesocket(SOCKET s);

CBSOCKET_PUBLIC_API
int get_socket_error();

CBSOCKET_PUBLIC_API
int bind(SOCKET sock, const struct sockaddr* name, socklen_t namelen);

CBSOCKET_PUBLIC_API
SOCKET accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen);

CBSOCKET_PUBLIC_API
int connect(SOCKET sock, const struct sockaddr* name, int namelen);

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

} // namespace net
} // namespace cb

#endif
