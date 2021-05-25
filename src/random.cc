/*
 *     Copyright 2014-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/random.h>

#include <cerrno>
#include <chrono>
#include <memory>
#include <mutex>
#include <system_error>

#ifdef WIN32
// We need to include windows.h _before_ wincrypt.h
#include <windows.h>

#include <wincrypt.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace cb {
class RandomGeneratorProvider {
public:
    RandomGeneratorProvider() {
#ifdef WIN32
        if (!CryptAcquireContext(&handle,
                                 NULL,
                                 NULL,
                                 PROV_RSA_FULL,
                                 CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
            throw std::system_error(int(GetLastError()),
                                    std::system_category(),
                                    "RandomGeneratorProvider::Failed to "
                                    "initialize random generator");
        }
#else
        if ((handle = open("/dev/urandom", O_RDONLY)) == -1) {
            throw std::system_error(errno,
                                    std::system_category(),
                                    "RandomGeneratorProvider::Failed to "
                                    "initialize random generator");
        }
#endif
    }

    virtual ~RandomGeneratorProvider() {
#ifdef WIN32
        CryptReleaseContext(handle, 0);
#else
        close(handle);
#endif
    }

    bool getBytes(void* dest, size_t size) {
        std::lock_guard<std::mutex> lock(mutex);
#ifdef WIN32
        return CryptGenRandom(handle, (DWORD)size, static_cast<BYTE*>(dest));
#else
        return size_t(read(handle, dest, size)) == size;
#endif
    }

protected:
#ifdef WIN32
    HCRYPTPROV handle{};
#else
    int handle = -1;
#endif
    std::mutex mutex;
};

RandomGenerator::RandomGenerator() = default;

uint64_t RandomGenerator::next() {
    uint64_t ret;
    if (getBytes(&ret, sizeof(ret))) {
        return ret;
    }

    return std::chrono::steady_clock::now().time_since_epoch().count();
}

bool RandomGenerator::getBytes(void* dest, size_t size) {
    return getInstance().getBytes(dest, size);
}

RandomGeneratorProvider& RandomGenerator::getInstance() {
    static RandomGeneratorProvider provider;
    return provider;
}

} // namespace Couchbase
