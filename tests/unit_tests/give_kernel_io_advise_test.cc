/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <fmt/format.h>
#include <folly/portability/GTest.h>
#include <platform/dirutils.h>
#include <platform/io_hint.h>

using namespace cb::io;

class IoAdvise : public ::testing::Test {
public:
    static void SetUpTestCase() {
        filename = cb::io::mktemp("IoAdvise");
        fp = fopen(filename.generic_string().c_str(), "r");
        ASSERT_TRUE(fp != nullptr)
                << "Failed to open file for reading: " << filename;
    }

    static void TearDownTestCase() {
        if (fp) {
            fclose(fp);
        }
        remove(filename);
    }

protected:
    static std::filesystem::path filename;
    static FILE* fp;
};

std::filesystem::path IoAdvise::filename;
FILE* IoAdvise::fp = nullptr;

TEST_F(IoAdvise, giveKernelIoAdviseNormal) {
    giveKernelIoAdvise(fileno(fp), IoHint::Normal);
}

TEST_F(IoAdvise, giveKernelIoAdviseSequential) {
    giveKernelIoAdvise(fileno(fp), IoHint::Sequential);
}

TEST_F(IoAdvise, giveKernelIoAdviseRandom) {
    giveKernelIoAdvise(fileno(fp), IoHint::Random);
}

TEST_F(IoAdvise, giveKernelIoAdviseNoReuse) {
    giveKernelIoAdvise(fileno(fp), IoHint::NoReuse);
}

TEST_F(IoAdvise, giveKernelIoAdviseWillNeed) {
    giveKernelIoAdvise(fileno(fp), IoHint::WillNeed);
}

TEST_F(IoAdvise, giveKernelIoAdviseDontNeed) {
    giveKernelIoAdvise(fileno(fp), IoHint::DontNeed);
}
