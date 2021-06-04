/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/**
 * This is a wrapping layer on top of BSD sockets to hide away the difference
 * between Windows and Linux (the first use char pointers and the latter
 * use void etc). This allows us to hide all of the platform difference in
 * one place and not have to do #ifdef's all over the place
 */
#pragma once

#include <platform/platform_socket.h>

#ifdef WIN32
typedef int in_port_t;
#define SOCKETPAIR_AF AF_INET
#else
#include <netinet/in.h> // For in_port_t
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SOCKETPAIR_AF AF_UNIX
#endif

#include <nlohmann/json_fwd.hpp>
#include <cerrno>
#include <cstdint>
#include <string>

namespace cb::net {

int closesocket(SOCKET s);

int get_socket_error();

int bind(SOCKET sock, const struct sockaddr* name, socklen_t namelen);

SOCKET accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen);

int connect(SOCKET sock, const struct sockaddr* name, size_t namelen);

SOCKET socket(int domain, int type, int protocol);

int shutdown(SOCKET sock, int how);

ssize_t send(SOCKET sock, const void* buffer, size_t length, int flags);

ssize_t sendmsg(SOCKET sock, const struct msghdr* message, int flags);

ssize_t sendto(SOCKET sock,
               const void* buffer,
               size_t length,
               int flags,
               const struct sockaddr* dest_addr,
               socklen_t dest_len);

ssize_t recv(SOCKET sock, void* buffer, size_t length, int flags);

ssize_t recvfrom(SOCKET sock,
                 void* buffer,
                 size_t length,
                 int flags,
                 struct sockaddr* address,
                 socklen_t* address_len);

ssize_t recvmsg(SOCKET sock, struct msghdr* message, int flags);

int getsockopt(SOCKET sock,
               int level,
               int option_name,
               void* option_value,
               socklen_t* option_len);

int setsockopt(SOCKET sock,
               int level,
               int option_name,
               const void* option_value,
               socklen_t option_len);

int socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]);

int set_socket_noblocking(SOCKET sock);

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
    return  (dw == ENOTCONN || dw == ECONNRESET);
}
inline int is_addrinuse(int dw = get_socket_error()) {
    return (dw == EADDRINUSE);
}
inline void set_ewouldblock() {
    errno = EWOULDBLOCK;
}
inline void set_econnreset() {
    errno = ECONNRESET;
}
#endif

/**
 * Get a textual representation of the address represented in sockaddr_storage
 */
std::string to_string(const struct sockaddr_storage* addr, socklen_t addr_len);

nlohmann::json to_json(const struct sockaddr_storage* addr, socklen_t addr_len);

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
std::string getsockname(SOCKET sfd);

/**
 * Get the name of the socket in a textual form like:
 *
 *    { "ip" : "::1", "port" : 11210 }
 *
 * @param sfd The socket to get the name for
 * @throws std::exception if one of the functions needed to look up the socket
 *                        name fails
 */
nlohmann::json getSockNameAsJson(SOCKET sfd);

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
std::string getpeername(SOCKET sfd);

/**
 * Get the name of the socket in a textual form like:
 *
 *    { "ip" : "::1", "port" : 11210 }
 *
 * @param sfd The socket to get the name for
 * @throws std::exception if one of the functions needed to look up the socket
 *                        name fails
 */
nlohmann::json getPeerNameAsJson(SOCKET sfd);

/**
 * Get all IPv4 and IPv6 addresses configured on this machine
 *
 * @return the first vector contains IPv4 addresses, the second Iv6
 *         addresses
 * @throws std::system_error if we fail to get the interface description
 */
std::pair<std::vector<std::string>, std::vector<std::string>> getIpAddresses(
        bool skipLoopback);

/**
 * Get the hostname for the machine
 *
 * @throws std::system_error if an error occurs
 */
std::string getHostname();

} // namespace cb::net
