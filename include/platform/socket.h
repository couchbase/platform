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
 * @todo open up for a callback to be called to allow the app to
 * determine if a particular socket should be logged or not (only if
 * the global logger flag is set)
 *
 * @todo As each application may open and close a lot of sockets during
 * its lifetime, the current version _deletes_ the files when the application
 * close the socket. The use case when I developed this was that the program
 * would crash and generate a coredump (but it could open/send-receive/close
 * a lot of data which could generate tens of GB of data first).
 *
 * One could use tcpdump to collect the same info, but I failed to get
 * tcpdump create a pcap file I know how to parse when using the loopback
 * interface. Whipping up this seemed a lot easier ;-)
 */

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int in_port_t;
typedef int sa_family_t;
#define SOCKETPAIR_AF AF_INET
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
void set_on_close_handler(void (* callback)(SOCKET, const std::string& dir));

/**
 * Enable / disable socket logging.
 *
 * When enabled each socket creates a directory in /tmp by using the
 * following name `/tmp/<pid>-<socketfd>`. In that directory there will
 * be one file named `r` containing all data read by the application, and
 * one file named `w` containing all data written by the application.
 *
 * @param enable set to true to start logging
 */
CBSOCKET_PUBLIC_API
void set_socket_logging(bool enable);

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
int socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]);

CBSOCKET_PUBLIC_API
int set_socket_noblocking(SOCKET sock);

} // namespace net
} // namespace cb

#endif
