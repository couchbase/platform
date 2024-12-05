/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/dirutils.h>
#include <fstream>

namespace cb::io {
void saveFile(const std::filesystem::path& path,
              std::string_view content,
              std::ios_base::openmode mode) {
    std::ofstream file;
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    file.open(path.string().c_str(), mode);
    file.write(content.data(), content.size());
    file.flush();
    file.close();
}

bool saveFile(const std::filesystem::path& path,
              std::string_view content,
              std::error_code& ec,
              std::ios_base::openmode mode) noexcept {
    try {
        saveFile(path, content, mode);
    } catch (const std::system_error& e) {
        ec = e.code();
        return false;
    } catch (const std::bad_alloc&) {
        ec = std::make_error_code(std::errc::not_enough_memory);
        return false;
    } catch (const std::exception&) {
        // This isn't exactly right, but good enough for now
        ec = std::make_error_code(std::errc::io_error);
        return false;
    }
    return true;
}
} // namespace cb::io