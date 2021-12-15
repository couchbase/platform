/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once
#include <memory>
#include <string>

namespace cb::cgroup {
class ControlGroup;
namespace priv {
// Allow the root of the filesystem to be passed in to allow for unit
// tests
std::unique_ptr<ControlGroup> make_control_group(std::string root = "");
} // namespace priv
} // namespace cb::cgroup