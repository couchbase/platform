/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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

// See https://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
#define _USE_MATH_DEFINES

// Include the histogram header first to ensure that it is standalone
#include <platform/histogram.h>

#include <cmath>
#include <algorithm>
#include <functional>
#include <sstream>
#include <thread>

#include "config.h"
#include <folly/portability/GTest.h>

class PopulatedSamples {
public:
    PopulatedSamples(std::ostream& stream)
        : s(stream) { }

    void operator()(const Histogram<int>::value_type& b) {
        if (b->count() > 0) {
            s << *b << "; ";
        }
    }

    std::ostream& s;
};

TEST(HistoTest, Basic) {
    GrowingWidthGenerator<int> gen(0, 10, M_E);
    Histogram<int> histo(gen, 10);
    histo.add(3, 1);
    histo.add(-3, 15);
    histo.add(84477242, 11);

    // Verify the stream stuff works.
    std::stringstream s;
    s << histo;
    std::string expected("{Histogram: [-2147483648, 0) = 15, [0, 10) = 1, "
                         "[10, 37) = 0, [37, 110) = 0, [110, 310) = 0, "
                         "[310, 855) = 0, [855, 2339) = 0, [2339, 6373) = 0, "
                         "[6373, 17339) = 0, [17339, 47148) = 0, "
                         "[47148, 128178) = 0, [128178, 2147483647) = 11}");
    EXPECT_EQ(expected, s.str());

    std::stringstream s2;
    PopulatedSamples ps(s2);
    std::for_each(histo.begin(), histo.end(), ps);
    expected = "[-2147483648, 0) = 15; [0, 10) = 1; [128178, 2147483647) = 11; ";
    EXPECT_EQ(expected, s2.str());
    EXPECT_EQ(27u, histo.total());

    // I haven't set a 4, but there should be something in that bin.
    EXPECT_EQ(1u, histo.getBin(4)->count());

    histo.reset();
    EXPECT_EQ(0u, histo.total());
}

TEST(HistoTest, Exponential) {
    ExponentialGenerator<int> gen(0, 10.0);
    Histogram<int> histo(gen, 5);
    std::string expected("{Histogram: [-2147483648, 1) = 0, [1, 10) = 0, "
                         "[10, 100) = 0, [100, 1000) = 0, [1000, 10000) = 0, "
                         "[10000, 100000) = 0, [100000, 2147483647) = 0}");
    std::stringstream s;
    s << histo;
    EXPECT_EQ(expected, s.str());
}

TEST(HistoTest, CompleteRange) {
    GrowingWidthGenerator<uint16_t> gen(0, 10, M_E);
    Histogram<uint16_t> histo(gen, 10);
    uint16_t i(0);
    do {
        histo.add(i);
        ++i;
    } while (i != 0);
}


TEST(BlockTimerTest, Basic) {
    MicrosecondHistogram histo;
    ASSERT_EQ(0u, histo.total());
    {
        BlockTimer timer(&histo);
    }
    EXPECT_EQ(1u, histo.total());
}

// TODO: This doesn't fully test the threshold functionality as it doesn't
// check that the threshold is actually logged.
TEST(BlockTimerTest, ThresholdTest) {
    MicrosecondHistogram histo;
    ASSERT_EQ(0u, histo.total());
    {
        GenericBlockTimer<MicrosecondHistogram, 1> timer(&histo, "thresholdTest");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    EXPECT_EQ(1u, histo.total());
}

TEST(MoveTest, Basic){
    Histogram<int> histo;
    std::stringstream s;
    s << histo;
    std::string oldExpected = s.str();
    s.str(std::string());
    Histogram<int> newHist(std::move(histo));
    s << newHist;
    ASSERT_EQ(oldExpected, s.str());
    s.str(std::string());
    s << histo;
    ASSERT_NE(s.str(), oldExpected);
}
