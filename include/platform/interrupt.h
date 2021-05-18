/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once


namespace cb::console {
using interrupt_handler = void (*)();

/**
 * Sets a handler that will be invoked when a ctrl+c event is detected
 *
 * @throws std::system_error if an error occurs while setting up the
 *                           handler
 */
void set_sigint_handler(interrupt_handler h);

/**
 * Clears any handler that might have been invoked where a ctrl+c event
 * was detected.
 */
void clear_sigint_handler();
} // namespace cb::console
