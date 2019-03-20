/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc.
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
#include <stdio.h>
#include <stdlib.h>

PLATFORM_PUBLIC_API
void cb_initialize_sockets(void)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        fprintf(stderr, "Socket Initialization Error. Program aborted\r\n");
        exit(EXIT_FAILURE);
    }
}

PLATFORM_PUBLIC_API
int sendmsg(SOCKET sock, const struct msghdr *msg, int flags)
{
    /* @todo make this more optimal! */
    int ii;
    int ret = 0;

    for (ii = 0; ii < msg->msg_iovlen; ++ii) {
        if (msg->msg_iov[ii].iov_len > 0) {
            int nw = send(sock, msg->msg_iov[ii].iov_base, (int)msg->msg_iov[ii].iov_len, flags);
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
