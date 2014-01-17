/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2014 Couchbase, Inc
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
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include <platform/platform.h>
#include <platform/random.h>

using namespace Couchbase;
using namespace std;

static int test_c_interface(void) {
    cb_rand_t r;
    char buffer[1024];
    size_t ii;

    if (cb_rand_open(&r) == -1) {
        cerr << "Failed to initialize random generator" << endl;
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    if (cb_rand_get(r, buffer, sizeof(buffer)) == -1) {
        cerr << "Failed to read random bytes" << endl;
        cb_rand_close(r);
        return -1;
    }

    /* In theory it may return 1k of 0 so we may have a false positive */
    for (ii = 0; ii < (size_t)sizeof(buffer); ++ii) {
        if (buffer[ii] != 0) {
            break;
        }
    }
    if (ii == (size_t)sizeof(buffer)) {
        cerr << "I got 1k of 0 (or it isn't working)" << endl;
        cb_rand_close(r);
        return -1;
    }

    if (cb_rand_close(r) == -1) {
        cerr << "rand close failed" << endl;
        return -1;
    }

    return 0;
}

static int basic_rand_tests(RandomGenerator *r1,  RandomGenerator *r2) {
   uint64_t v1 = r1->next();
   uint64_t v2 = r2->next();
   if (v1 == v2) {
       cerr << "I did not expect the random generators to return the"
                 << " same value" << endl;
       return -1;
   }

   char buffer[1024];
   memset(buffer, 0, sizeof(buffer));
   if (r1->getBytes(buffer, sizeof(buffer))) {
       size_t ii;

       for (ii = 0; ii < (size_t)sizeof(buffer); ++ii) {
           if (buffer[ii] != 0) {
               break;
           }
       }

       if (ii == (size_t)sizeof(buffer)) {
           cerr << "I got 1k of 0 (or it isn't working)" << endl;

           return -1;
       }
   }

   return 0;
}

static int test_cc_interface(void) {
   RandomGenerator *r1 = new RandomGenerator(false);
   RandomGenerator *r2 = new RandomGenerator(false);

   if (r1->getProvider() == r2->getProvider()) {
       cerr << "The random generators should not share a provider"
                 << endl;
       return -1;
   }

   if (basic_rand_tests(r1, r2) != 0) {
       return -1;
   }

   delete r1;
   delete r2;

   r1 = new RandomGenerator(true);
   r2 = new RandomGenerator(true);

   if (r1->getProvider() != r2->getProvider()) {
       cerr << "The shared random generators should share a provider"
                 << endl;
       return -1;
   }

   if (basic_rand_tests(r1, r2) != 0) {
       return -1;
   }

   delete r1;
   delete r2;

   return 0;
}

int main(int argc, char **argv)
{
   int c_error = test_c_interface();
   int cc_error = test_cc_interface();

   return (c_error== 0 && cc_error == 0) ? 0 : -1;
}
