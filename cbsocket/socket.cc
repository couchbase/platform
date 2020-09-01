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

#include <platform/platform_socket.h>
#include <platform/socket.h>

#include <event2/util.h>
#include <nlohmann/json.hpp>

#include <cerrno>
#include <gsl/gsl>

#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#endif

namespace cb {
namespace net {

CBSOCKET_PUBLIC_API
int closesocket(SOCKET s) {
#ifdef WIN32
    return ::closesocket(s);
#else
    return ::close(s);
#endif

}

CBSOCKET_PUBLIC_API
int get_socket_error() {
#ifdef WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

CBSOCKET_PUBLIC_API
int bind(SOCKET sock, const struct sockaddr* name, socklen_t namelen) {
    return ::bind(sock, name, namelen);
}

CBSOCKET_PUBLIC_API
int listen(SOCKET sock, int backlog) {
    return ::listen(sock, backlog);
}


CBSOCKET_PUBLIC_API
SOCKET accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen) {
    return ::accept(sock, addr, addrlen);
}

CBSOCKET_PUBLIC_API
int connect(SOCKET sock, const struct sockaddr* name, size_t namelen) {
#ifdef WIN32
    return ::connect(sock, name, gsl::narrow<int>(namelen));
#else
    return ::connect(sock, name, gsl::narrow<socklen_t>(namelen));
#endif
}

CBSOCKET_PUBLIC_API
SOCKET socket(int domain, int type, int protocol) {
    return ::socket(domain, type, protocol);
}

CBSOCKET_PUBLIC_API
int shutdown(SOCKET sock, int how) {
    return ::shutdown(sock, how);
}

CBSOCKET_PUBLIC_API
ssize_t send(SOCKET sock, const void* buffer, size_t length, int flags) {
#ifdef WIN32
    return ::send(sock, static_cast<const char*>(buffer), gsl::narrow<int>(length), flags);
#else
    return ::send(sock, buffer, length, flags);
#endif
}

CBSOCKET_PUBLIC_API
ssize_t sendmsg(SOCKET sock, const struct msghdr* message, int flags) {
    return ::sendmsg(sock, message, flags);
}

CBSOCKET_PUBLIC_API
ssize_t sendto(SOCKET sock,
               const void* buffer,
               size_t length,
               int flags,
               const struct sockaddr* dest_addr,
               socklen_t dest_len) {
#ifdef WIN32
    return ::sendto(sock,
                    static_cast<const char*>(buffer),
                    int(length),
                    flags,
                    dest_addr,
                    dest_len);
#else
    return ::sendto(sock, buffer, length, flags, dest_addr, dest_len);
#endif
}

CBSOCKET_PUBLIC_API
ssize_t recv(SOCKET sock, void* buffer, size_t length, int flags) {
#ifdef WIN32
    return ::recv(sock, static_cast<char*>(buffer), gsl::narrow<int>(length), flags);
#else
    return ::recv(sock, buffer, length, flags);
#endif
}

CBSOCKET_PUBLIC_API
ssize_t recvfrom(SOCKET sock,
                 void* buffer,
                 size_t length,
                 int flags,
                 struct sockaddr* address,
                 socklen_t* address_len) {
#ifdef WIN32
    return ::recvfrom(sock,
                      static_cast<char*>(buffer),
                      int(length),
                      flags,
                      address,
                      address_len);
#else
    return ::recvfrom(sock, buffer, length, flags, address, address_len);
#endif
}

CBSOCKET_PUBLIC_API
ssize_t recvmsg(SOCKET sock, struct msghdr* message, int flags) {
#ifdef WIN32
    int res = 0;
    for (size_t ii = 0; ii < size_t(message->msg_iovlen); ii++) {
        auto nr = cb::net::recv(sock,
                                message->msg_iov[ii].iov_base,
                                message->msg_iov[ii].iov_len,
                                0);
        if (nr == -1) {
            return (res == 0) ? -1 : res;
        }

        res += nr;
    }

    return res;
#else
    return ::recvmsg(sock, message, flags);
#endif
}

CBSOCKET_PUBLIC_API
int socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]) {
    return evutil_socketpair(domain,
                             type,
                             protocol,
                             reinterpret_cast<evutil_socket_t*>(socket_vector));
}

CBSOCKET_PUBLIC_API
int getsockopt(SOCKET sock,
               int level,
               int option_name,
               void* option_value,
               socklen_t* option_len) {
#ifdef WIN32
    return ::getsockopt(sock,
                        level,
                        option_name,
                        reinterpret_cast<char*>(option_value),
                        option_len);
#else
    return ::getsockopt(sock, level, option_name, option_value, option_len);
#endif
}

CBSOCKET_PUBLIC_API
int setsockopt(SOCKET sock,
               int level,
               int option_name,
               const void* option_value,
               socklen_t option_len) {
#ifdef WIN32
    return ::setsockopt(sock,
                        level,
                        option_name,
                        reinterpret_cast<const char*>(option_value),
                        option_len);
#else
    return ::setsockopt(sock, level, option_name, option_value, option_len);
#endif
}

CBSOCKET_PUBLIC_API
int set_socket_noblocking(SOCKET sock) {
    return evutil_make_socket_nonblocking(sock);
}

CBSOCKET_PUBLIC_API
std::string to_string(const struct sockaddr_storage* addr, socklen_t addr_len) {
    char host[50];
    char port[50];

    int err = getnameinfo(reinterpret_cast<const struct sockaddr*>(addr),
                          addr_len,
                          host,
                          sizeof(host),
                          port,
                          sizeof(port),
                          NI_NUMERICHOST | NI_NUMERICSERV);
    if (err != 0) {
        throw std::runtime_error(
                "cb::net::to_string: getnameinfo() failed with error: " +
                std::to_string(err));
    }

    if (addr->ss_family == AF_INET6) {
        return "[" + std::string(host) + "]:" + std::string(port);
    } else {
        return std::string(host) + ":" + std::string(port);
    }
}

CBSOCKET_PUBLIC_API
nlohmann::json to_json(const struct sockaddr_storage* addr,
                       socklen_t addr_len) {
    std::array<char, 50> host;
    std::array<char, 50> port;

    int err = getnameinfo(reinterpret_cast<const struct sockaddr*>(addr),
                          addr_len,
                          host.data(),
                          host.size(),
                          port.data(),
                          port.size(),
                          NI_NUMERICHOST | NI_NUMERICSERV);
    if (err != 0) {
        throw std::runtime_error(
                "cb::net::to_json: getnameinfo() failed with error: " +
                std::to_string(err));
    }

    return nlohmann::json{{"ip", host.data()},
                          {"port", std::stoi(port.data())}};
}

CBSOCKET_PUBLIC_API
std::string getsockname(SOCKET sfd) {
    sockaddr_storage sock{};
    socklen_t sock_len = sizeof(sock);
    if (::getsockname(sfd,
                      reinterpret_cast<struct sockaddr*>(&sock),
                      &sock_len) != 0) {
        throw std::system_error(cb::net::get_socket_error(),
                                std::system_category(),
                                "getsockname() failed");
    }
    return to_string(&sock, sock_len);
}

CBSOCKET_PUBLIC_API
nlohmann::json getSockNameAsJson(SOCKET sfd) {
    sockaddr_storage sock{};
    socklen_t sock_len = sizeof(sock);
    if (::getsockname(sfd,
                      reinterpret_cast<struct sockaddr*>(&sock),
                      &sock_len) != 0) {
        throw std::system_error(cb::net::get_socket_error(),
                                std::system_category(),
                                "getsockname() failed");
    }
    return to_json(&sock, sock_len);
}

CBSOCKET_PUBLIC_API
std::string getpeername(SOCKET sfd) {
    sockaddr_storage peer;
    socklen_t peer_len = sizeof(peer);
    if (getpeername(sfd,
                    reinterpret_cast<struct sockaddr*>(&peer),
                    &peer_len) != 0) {
        throw std::system_error(cb::net::get_socket_error(),
                                std::system_category(),
                                "getpeername() failed");
    }
    return to_string(&peer, peer_len);
}

CBSOCKET_PUBLIC_API
nlohmann::json getPeerNameAsJson(SOCKET sfd) {
    sockaddr_storage peer;
    socklen_t peer_len = sizeof(peer);
    if (getpeername(sfd,
                    reinterpret_cast<struct sockaddr*>(&peer),
                    &peer_len) != 0) {
        throw std::system_error(cb::net::get_socket_error(),
                                std::system_category(),
                                "getpeername() failed");
    }
    return to_json(&peer, peer_len);
}

} // namespace net
} // namespace cb
