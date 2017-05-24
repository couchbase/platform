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

#include <platform/socket.h>

#include <platform/dirutils.h>
#include <platform/platform.h>

#include <atomic>
#include <memory>
#include <string>
#include <string.h>
#include <system_error>

#ifndef WIN32
#include <unistd.h>
#include <cerrno>
#include <netdb.h>

#endif


struct FileDeleter {
    void operator()(FILE* fp) { fclose(fp); }
};

using unique_file_ptr = std::unique_ptr<FILE, FileDeleter>;


std::string get_directory(SOCKET socket) {
    std::string name{"/tmp/"};
    name.append(std::to_string(cb_getpid()));
    name.append("-");
    name.append(std::to_string(socket));
    return name;
}

static std::atomic_bool logging;

static inline bool should_log(SOCKET sock) {
    (void)sock;
    return logging.load(std::memory_order_relaxed);

#if 0
    // If you want to do some more filtering (for instance only log traffic
    // to certain ports you may just modify this piece of code...
    //
    // This example will only enable looging where the peer port must be 11210
    struct sockaddr_storage peer{};
    socklen_t peer_len = sizeof(peer);
    int err;
    if ((err = getpeername(sock,
                           reinterpret_cast<struct sockaddr*>(&peer),
                           &peer_len)) != 0) {
        return false;
    }

    char host[50];
    char port[50];

    err = getnameinfo(reinterpret_cast<struct sockaddr*>(&peer),
                          peer_len,
                          host, sizeof(host),
                          port, sizeof(port),
                          NI_NUMERICHOST | NI_NUMERICSERV);
    if (err != 0) {
        return false;
    }

    return (atoi(port) == 11210);
#endif
}

static void log_it(SOCKET sock, const void* data, size_t nb, const char* dir) {
    std::string fname = get_directory(sock) + dir;
    unique_file_ptr fp(fopen(fname.c_str(), "ab"));
    if (!fp) {
        if (errno == ENOENT) {
            // The parent directory may not exists (if the socket was
            // created with lets say any of the libevent methods
            // (evutil_socketpair for instance).
            return;
        }
        throw std::system_error(errno,
                                std::system_category(),
                                "Failed to open socket log file " + fname);
    }

    auto nw = fwrite(data, 1, nb, fp.get());
    if (nw != nb) {
        throw std::system_error(
            errno,
            std::system_category(),
            "Incorrect number of bytes written to socket log file");
    }
}


namespace cb {
namespace net {

CBSOCKET_PUBLIC_API
void set_socket_logging(bool enable) {
    logging.store(enable);
}

CBSOCKET_PUBLIC_API
int closesocket(SOCKET s) {
    int ret;
#ifdef WIN32
    ret = ::closesocket(s);
#else
    ret = ::close(s);
#endif

    if (ret == 0 && logging) {
        try {
            cb::io::rmrf(get_directory(s));
        } catch (...) {
        }
    }

    return ret;
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
SOCKET accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen) {
    auto ret = ::accept(sock, addr, addrlen);
    if (ret != INVALID_SOCKET && logging) {
        cb::io::mkdirp(get_directory(ret));
    }

    return ret;
}

CBSOCKET_PUBLIC_API
int connect(SOCKET sock, const struct sockaddr* name, int namelen) {
    return ::connect(sock, name, namelen);
}

CBSOCKET_PUBLIC_API
SOCKET socket(int domain, int type, int protocol) {
    auto ret = ::socket(domain, type, protocol);
    if (ret != INVALID_SOCKET && logging) {
        cb::io::mkdirp(get_directory(ret));
    }
    return ret;
}

CBSOCKET_PUBLIC_API
int shutdown(SOCKET sock, int how) {
    return ::shutdown(sock, how);
}

CBSOCKET_PUBLIC_API
ssize_t send(SOCKET sock, const void* buffer, size_t length, int flags) {
#ifdef WIN32
    auto ret =
            ::send(sock, static_cast<const char*>(buffer), int(length), flags);
#else
    auto ret = ::send(sock, buffer, length, flags);
#endif
    if (ret > 0 && should_log(sock)) {
        log_it(sock, buffer, size_t(ret), "/w");
    }

    return ret;
}

CBSOCKET_PUBLIC_API
ssize_t sendmsg(SOCKET sock, const struct msghdr* message, int flags) {
    if (logging) {
        int res = 0;
        for (size_t ii = 0; ii < size_t(message->msg_iovlen); ii++) {
            auto nw = cb::net::send(sock,
                                    message->msg_iov[ii].iov_base,
                                    message->msg_iov[ii].iov_len,
                                    0);
            if (nw == -1) {
                return (res == 0) ? -1 : res;
            }

            res += nw;
        }

        return res;
    }

    return ::sendmsg(sock, message, flags);
}

CBSOCKET_PUBLIC_API
ssize_t sendto(SOCKET sock,
               const void* buffer,
               size_t length,
               int flags,
               const struct sockaddr* dest_addr,
               socklen_t dest_len) {
    if (logging) {
        throw std::runtime_error("cb::net::sendto: not implemented");
    }

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
    auto ret = ::recv(sock, static_cast<char*>(buffer), length, flags);
#else
    auto ret = ::recv(sock, buffer, length, flags);
#endif

    if (ret > 0 && should_log(sock)) {
        log_it(sock, buffer, size_t(ret), "/r");
    }

    return ret;
}

CBSOCKET_PUBLIC_API
ssize_t recvfrom(SOCKET sock,
                 void* buffer,
                 size_t length,
                 int flags,
                 struct sockaddr* address,
                 socklen_t* address_len) {
    if (logging) {
        throw std::runtime_error("cb::net::recvfrom: not implemented");
    }

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
    if (logging) {
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
    }

#ifdef WIN32
    throw std::runtime_error("cb::net::recvmsg: Not implemented for win32");
#else
    return ::recvmsg(sock, message, flags);
#endif
}

} // namespace net
} // namespace cb

// C API which wraps into the C++ API

CBSOCKET_PUBLIC_API
int cb_closesocket(SOCKET s) {
    return cb::net::closesocket(s);
}

CBSOCKET_PUBLIC_API
int cb_get_socket_error() {
    return cb::net::get_socket_error();
}

CBSOCKET_PUBLIC_API
int cb_bind(SOCKET sock, const struct sockaddr* name, socklen_t namelen) {
    return cb::net::bind(sock, name, namelen);
}

CBSOCKET_PUBLIC_API
SOCKET cb_accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen) {
    return cb::net::accept(sock, addr, addrlen);
}

CBSOCKET_PUBLIC_API
int cb_connect(SOCKET sock, const struct sockaddr* name, int namelen) {
    return cb::net::connect(sock, name, namelen);
}

CBSOCKET_PUBLIC_API
SOCKET cb_socket(int domain, int type, int protocol) {
    return cb::net::socket(domain, type, protocol);
}

CBSOCKET_PUBLIC_API
int cb_shutdown(SOCKET sock, int how) {
    return cb::net::shutdown(sock, how);
}

CBSOCKET_PUBLIC_API
ssize_t cb_send(SOCKET sock, const void* buffer, size_t length, int flags) {
    return cb::net::send(sock, buffer, length, flags);
}

CBSOCKET_PUBLIC_API
ssize_t cb_sendmsg(SOCKET sock, const struct msghdr* message, int flags) {
    return cb::net::sendmsg(sock, message, flags);
}

CBSOCKET_PUBLIC_API
ssize_t cb_sendto(SOCKET sock,
                  const void* buffer,
                  size_t length,
                  int flags,
                  const struct sockaddr* dest_addr,
                  socklen_t dest_len) {
    return cb::net::sendto(sock, buffer, length, flags, dest_addr, dest_len);
}

CBSOCKET_PUBLIC_API
ssize_t cb_recv(SOCKET sock, void* buffer, size_t length, int flags) {
    return cb::net::recv(sock, buffer, length, flags);
}

CBSOCKET_PUBLIC_API
ssize_t cb_recvfrom(SOCKET sock,
                    void* buffer,
                    size_t length,
                    int flags,
                    struct sockaddr* address,
                    socklen_t* address_len) {
    return cb::net::recvfrom(sock, buffer, length, flags, address, address_len);
}

CBSOCKET_PUBLIC_API
ssize_t cb_recvmsg(SOCKET sock, struct msghdr* message, int flags) {
    return cb::net::recvmsg(sock, message, flags);
}

CBSOCKET_PUBLIC_API
void cb_set_socket_logging(bool enable) {
    cb::net::set_socket_logging(enable);
}
