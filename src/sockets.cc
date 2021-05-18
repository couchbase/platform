/*
 *     Copyright 2013-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/platform_socket.h>

#ifdef WIN32

#include <stdio.h>
#include <stdlib.h>

void cb_initialize_sockets(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        fprintf(stderr, "Socket Initialization Error. Program aborted\r\n");
        exit(EXIT_FAILURE);
    }
}

int sendmsg(SOCKET sock, const struct msghdr* msg, int flags) {
    /* @todo make this more optimal! */
    int ii;
    int ret = 0;

    for (ii = 0; ii < msg->msg_iovlen; ++ii) {
        if (msg->msg_iov[ii].iov_len > 0) {
            int nw = ::send(sock,
                            static_cast<const char*>(msg->msg_iov[ii].iov_base),
                            (int)msg->msg_iov[ii].iov_len,
                            flags);
            if (nw > 0) {
                ret += nw;
                if (nw != msg->msg_iov[ii].iov_len) {
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
}

#endif // WIN32
