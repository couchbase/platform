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

#include <event2/util.h>

#include <atomic>
#include <cerrno>
#include <memory>
#include <mutex>
#include <string>
#include <string.h>
#include <system_error>
#include <unordered_map>

#ifndef WIN32
#include <unistd.h>
#include <netdb.h>
#endif

/**
 * We cache each directory name in an unordered map where we remove
 * the entry every time the socket is successfully closed. By
 * doing this we can create a new file every time the same socket
 * is reopened.
 */
std::mutex mutex;
std::unordered_map<SOCKET, std::string> name_map;

std::string get_directory(SOCKET socket) {
    std::lock_guard<std::mutex> guard(mutex);

    auto iter = name_map.find(socket);
    if (iter == name_map.end()) {
        std::string prefix{"/tmp/"};
        prefix.append(std::to_string(cb_getpid()));
        prefix.append("-");
        prefix.append(std::to_string(socket));
        prefix.append("-");

        std::string name;
        uint32_t seqno = 0;
        do {
            name = prefix+ std::to_string(seqno++);
        } while (cb::io::isDirectory(name));

        name_map[socket] = name;
        return name;
    }

    return iter->second;
}


static void log_it(SOCKET sock, const void* data, size_t nb, const char* dir) {
    struct FileDeleter {
        void operator()(FILE* fp) { fclose(fp); }
    };

    std::string fname = get_directory(sock) + dir;
    std::unique_ptr<FILE, FileDeleter> fp(fopen(fname.c_str(), "ab"));

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

static bool defaultFilterHandler(SOCKET sock) {
    (void)sock;
    return true;
}

static void defaultCloseHandler(SOCKET sock, const std::string& dir) {
    try {
        cb::io::rmrf(dir);
    } catch (...) {
        // Empty
    }
}

static std::atomic<bool(*)(SOCKET)> filter_handler {defaultFilterHandler};
static std::atomic<void(*)(SOCKET, const std::string&)> close_handler {defaultCloseHandler};
static std::atomic_bool logging;

static inline bool should_log(SOCKET sock) {
    if (!logging.load(std::memory_order_relaxed)) {
        return false;
    }

    // Logging is enabled.. check the filter for this socket
    return filter_handler.load()(sock);
}

namespace cb {
namespace net {

CBSOCKET_PUBLIC_API
void set_log_filter_handler(bool (* handler)(SOCKET)) {
    if (handler == nullptr) {
        filter_handler.store(defaultFilterHandler);
    } else {
        filter_handler.store(handler);
    }
}

CBSOCKET_PUBLIC_API
void set_on_close_handler(void (* handler)(SOCKET, const std::string& dir)) {
    if (handler == nullptr) {
        close_handler.store(defaultCloseHandler);
    } else {
        close_handler.store(handler);
    }
}

CBSOCKET_PUBLIC_API
void set_socket_logging(bool enable) {
    logging.store(enable);
}

CBSOCKET_PUBLIC_API
int closesocket(SOCKET s) {
    if (logging) {
        std::lock_guard<std::mutex> guard(mutex);
        int ret;
#ifdef WIN32
        ret = ::closesocket(s);
#else
        ret = ::close(s);
#endif

        if (ret == 0 && logging) {
            std::string name;
            auto iter = name_map.find(s);
            if (iter != name_map.end()) {
                name = iter->second;
                name_map.erase(iter);
                close_handler.load()(s, name);
            }
        }

        return ret;
    }

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
SOCKET accept(SOCKET sock, struct sockaddr* addr, socklen_t* addrlen) {
    auto ret = ::accept(sock, addr, addrlen);
    if (ret != INVALID_SOCKET && should_log(ret)) {
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
    if (ret != INVALID_SOCKET && should_log(ret)) {
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
    if (should_log(sock)) {
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
    if (should_log(sock)) {
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

CBSOCKET_PUBLIC_API
int socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]) {
    auto ret = evutil_socketpair(domain,
                                 type,
                                 protocol,
                                 reinterpret_cast<evutil_socket_t*>(socket_vector));
    if (ret == 0 && logging) {
        cb::io::mkdirp(get_directory(socket_vector[0]));
        cb::io::mkdirp(get_directory(socket_vector[1]));
    }
    return ret;
}

CBSOCKET_PUBLIC_API
int set_socket_noblocking(SOCKET sock) {
    return evutil_make_socket_nonblocking(sock);
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

CBSOCKET_PUBLIC_API
int cb_socketpair(int domain, int type, int protocol, SOCKET socket_vector[2]) {
    return cb::net::socketpair(domain, type, protocol, socket_vector);
}

CBSOCKET_PUBLIC_API
int cb_set_socket_noblocking(SOCKET sock) {
    return cb::net::set_socket_noblocking(sock);
}
