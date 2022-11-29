/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>

#include <platform/simd/scan.h>

static gsl::span<const unsigned char> testString = {
        reinterpret_cast<const unsigned char*>("0123456789ABCDEF"), 16};

TEST(ScanAnyOf128, SingleMatch) {
    EXPECT_EQ(0, cb::simd::scan_any_of_128bit<'0'>(testString));
    EXPECT_EQ(1, cb::simd::scan_any_of_128bit<'1'>(testString));
    EXPECT_EQ(2, cb::simd::scan_any_of_128bit<'2'>(testString));
    EXPECT_EQ(3, cb::simd::scan_any_of_128bit<'3'>(testString));
    EXPECT_EQ(4, cb::simd::scan_any_of_128bit<'4'>(testString));
    EXPECT_EQ(5, cb::simd::scan_any_of_128bit<'5'>(testString));
    EXPECT_EQ(6, cb::simd::scan_any_of_128bit<'6'>(testString));
    EXPECT_EQ(7, cb::simd::scan_any_of_128bit<'7'>(testString));
    EXPECT_EQ(8, cb::simd::scan_any_of_128bit<'8'>(testString));
    EXPECT_EQ(9, cb::simd::scan_any_of_128bit<'9'>(testString));
    EXPECT_EQ(10, cb::simd::scan_any_of_128bit<'A'>(testString));
    EXPECT_EQ(11, cb::simd::scan_any_of_128bit<'B'>(testString));
    EXPECT_EQ(12, cb::simd::scan_any_of_128bit<'C'>(testString));
    EXPECT_EQ(13, cb::simd::scan_any_of_128bit<'D'>(testString));
    EXPECT_EQ(14, cb::simd::scan_any_of_128bit<'E'>(testString));
    EXPECT_EQ(15, cb::simd::scan_any_of_128bit<'F'>(testString));
    // Missing bytes return 16.
    EXPECT_EQ(16, cb::simd::scan_any_of_128bit<'?'>(testString));
}

TEST(ScanAnyOf128, MultipleMatches) {
    EXPECT_EQ(1, (cb::simd::scan_any_of_128bit<'1', '2'>(testString)));
    EXPECT_EQ(1, (cb::simd::scan_any_of_128bit<'2', '1'>(testString)));
    EXPECT_EQ(1,
              (cb::simd::scan_any_of_128bit<'1', '2', '3', '4'>(testString)));
    EXPECT_EQ(16, (cb::simd::scan_any_of_128bit<'?', '!'>(testString)));
}
