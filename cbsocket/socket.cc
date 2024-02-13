/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/socket.h>

#include <event2/util.h>
#include <gsl/gsl-lite.hpp>
#include <nlohmann/json.hpp>
#include <platform/platform_socket.h>
#include <cerrno>

#ifdef WIN32
#include <winsock2.h>

#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace cb::net {

void initialize() {
#ifdef WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        fprintf(stderr, "Socket Initialization Error. Program aborted\r\n");
        exit(EXIT_FAILURE);
    }
#endif
}

int closesocket(SOCKET s) {
#ifdef WIN32
    return ::closesocket(s);
#else
    return ::close(s);
#endif

}

int get_socket_error() {
#ifdef WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

int bind(SOCKET sock, const struct sockaddr* name, socklen_t namelen) {
    return ::bind(sock, name, namelen);
}

int listen(SOCKET sock, int backlog) {
    return ::listen(sock, backlog);
}


SOCKET accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen) {
    return ::accept(sock, addr, addrlen);
}

int connect(SOCKET sock, const struct sockaddr* name, size_t namelen) {
#ifdef WIN32
    return ::connect(sock, name, gsl::narrow<int>(namelen));
#else
    return ::connect(sock, name, gsl::narrow<socklen_t>(namelen));
#endif
}

SOCKET socket(int domain, int type, int protocol) {
    return ::socket(domain, type, protocol);
}

int shutdown(SOCKET sock, int how) {
    return ::shutdown(sock, how);
}

ssize_t send(SOCKET sock, const void* buffer, size_t length, int flags) {
#ifdef WIN32
    return ::send(sock, static_cast<const char*>(buffer), gsl::narrow<int>(length), flags);
#else
    return ::send(sock, buffer, length, flags);
#endif
}

ssize_t sendmsg(SOCKET sock, const struct msghdr* message, int flags) {
#ifdef WIN32
    /* @todo make this more optimal! */
    int ii;
    int ret = 0;

    for (ii = 0; ii < message->msg_iovlen; ++ii) {
        if (message->msg_iov[ii].iov_len > 0) {
            int nw = ::send(
                    sock,
                    static_cast<const char*>(message->msg_iov[ii].iov_base),
                    (int)message->msg_iov[ii].iov_len,
                    flags);
            if (nw > 0) {
                ret += nw;
                if (nw != message->msg_iov[ii].iov_len) {
                    return ret;
                }
            } else {
                if (ret > 0) {
                    return ret;
                }
                return nw;
            }
        }
    }

    return ret;
#else
    return ::sendmsg(sock, message, flags);
#endif
}

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

ssize_t recv(SOCKET sock, void* buffer, size_t length, int flags) {
#ifdef WIN32
    return ::recv(sock, static_cast<char*>(buffer), gsl::narrow<int>(length), flags);
#else
    return ::recv(sock, buffer, length, flags);
#endif
}

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

int socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]) {
    return evutil_socketpair(domain,
                             type,
                             protocol,
                             reinterpret_cast<evutil_socket_t*>(socket_vector));
}

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

int set_socket_noblocking(SOCKET sock) {
    return evutil_make_socket_nonblocking(sock);
}

void set_socket_blocking(SOCKET sock) {
#ifdef WIN32
    DWORD imode = 0;
    auto st = ioctlsocket(sock, FIONBIO, &imode);
    if (st != 0) {
        if (st == SOCKET_ERROR) {
            throw std::system_error(
                    WSAGetLastError(), std::system_category(), "ioctlsocket");
        }
        throw std::system_error(st, std::system_category(), "ioctlsocket");
    }
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        throw std::system_error(
                errno, std::system_category(), "fcntl(F_GETFL)");
    }

    flags &= ~O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flags) == -1) {
        throw std::system_error(
                errno, std::system_category(), "fcntl(F_SETFL)");
    }
#endif
}

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

std::pair<std::vector<std::string>, std::vector<std::string>> getIpAddresses(
        bool skipLoopback) {
    std::array<char, 1024> buffer;
    std::pair<std::vector<std::string>, std::vector<std::string>> ret;

#ifdef WIN32
    std::vector<char> blob(1024 * 1024);
    auto* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(blob.data());
    ULONG dataSize = blob.size();
    auto rw = GetAdaptersAddresses(
            AF_UNSPEC,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                    GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
            nullptr,
            addresses,
            &dataSize);
    if (rw != ERROR_SUCCESS) {
        throw std::system_error(
                rw, std::system_category(), "GetAdaptersAddresses()");
    }
    for (auto* iff = addresses; iff != nullptr; iff = iff->Next) {
        if (iff->OperStatus != IfOperStatusUp) {
            // Interface not up
            continue;
        }
        for (auto* addr = iff->FirstUnicastAddress; addr != nullptr;
             addr = addr->Next) {
            auto family = addr->Address.lpSockaddr->sa_family;
            if (family == AF_INET) {
                inet_ntop(AF_INET,
                          &reinterpret_cast<sockaddr_in*>(
                                   addr->Address.lpSockaddr)
                                   ->sin_addr,
                          buffer.data(),
                          buffer.size());
                std::string address(buffer.data());
                if (!(skipLoopback && address == "127.0.0.1")) {
                    // Ignore localhost address
                    ret.first.emplace_back(std::move(address));
                }
            } else if (family == AF_INET6) {
                inet_ntop(AF_INET6,
                          &reinterpret_cast<sockaddr_in6*>(
                                   addr->Address.lpSockaddr)
                                   ->sin6_addr,
                          buffer.data(),
                          buffer.size());
                std::string address(buffer.data());
                if (!(skipLoopback && address == "::1")) {
                    // Ignore localhost address
                    ret.second.emplace_back(std::move(address));
                }
            } else {
                // Skip all other types of addresses
                continue;
            }
        }
    }
#else
    ifaddrs* interfaces;
    if (getifaddrs(&interfaces) != 0) {
        throw std::system_error(errno, std::system_category(), "getifaddrs()");
    }

    if (interfaces == nullptr) {
        return {};
    }

    // Iterate over all the interfaces and pick out the IPv4 and IPv6 addresses
    for (auto* ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) {
            inet_ntop(AF_INET,
                      &reinterpret_cast<sockaddr_in*>(ifa->ifa_addr)->sin_addr,
                      buffer.data(),
                      buffer.size());
            std::string address(buffer.data());
            if (!(skipLoopback && address == "127.0.0.1")) {
                // Ignore localhost address
                ret.first.emplace_back(std::move(address));
            }
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            inet_ntop(
                    AF_INET6,
                    &reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr)->sin6_addr,
                    buffer.data(),
                    buffer.size());
            std::string address(buffer.data());
            if (!(skipLoopback && address == "::1")) {
                // Ignore localhost address
                ret.second.emplace_back(std::move(address));
            }
        }
    }

    freeifaddrs(interfaces);
#endif

    return ret;
}

std::string getHostname() {
    std::array<char, 256> host;
    if (::gethostname(host.data(), host.size()) != 0) {
        throw std::system_error(
                get_socket_error(), std::system_category(), "gethostname()");
    }

    return std::string(host.data());
}

nlohmann::json getSocketOptions(SOCKET sfd) {
    nlohmann::json ret;
    auto add_option = [sfd, &ret](const char* key, int level, int option) {
        try {
            ret[key] = getSocketOption<int>(sfd, level, option);
        } catch (const std::exception& exception) {
            ret[key] = exception.what();
        }
    };

    add_option("so_sndbuf", SOL_SOCKET, SO_SNDBUF);
    add_option("so_rcvbuf", SOL_SOCKET, SO_RCVBUF);
    add_option("so_keepalive", SOL_SOCKET, SO_KEEPALIVE);
    add_option("tcp_keepidle", IPPROTO_TCP, TCP_KEEPIDLE);
    add_option("tcp_keepintvl", IPPROTO_TCP, TCP_KEEPINTVL);
    add_option("tcp_keepcnt", IPPROTO_TCP, TCP_KEEPCNT);
#ifdef __linux__
    add_option("tcp_user_timeout", IPPROTO_TCP, TCP_USER_TIMEOUT);
#endif

    try {
        auto linger =
                getSocketOption<struct linger>(sfd, SOL_SOCKET, SO_LINGER);
        if (linger.l_onoff) {
            ret["so_linger"] = linger.l_linger;
        } else {
            ret["so_linger"] = "off";
        }
    } catch (const std::exception& exception) {
        ret["so_linger"] = exception.what();
    }

    return ret;
}

} // namespace cb::net
