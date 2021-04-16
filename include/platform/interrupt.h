/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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
