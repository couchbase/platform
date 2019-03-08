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
#include "config.h"

#include <platform/random.h>
#include <platform/strerror.h>

#include <chrono>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace Couchbase {
   class RandomGeneratorProvider {

   public:
      RandomGeneratorProvider() {
         if (cb_rand_open(&provider) == -1) {
            std::stringstream ss;
            std::string reason = cb_strerror();
            ss << "Failed to initialize random generator: " << reason;
            throw std::runtime_error(ss.str());
         }
      }

      virtual ~RandomGeneratorProvider() {
         (void)cb_rand_close(provider);
      }

      virtual bool getBytes(void *dest, size_t size) {
         return cb_rand_get(provider, dest, size) == 0;
      }

   protected:
      cb_rand_t provider;
   };

   class SharedRandomGeneratorProvider : public RandomGeneratorProvider {
   public:
      virtual bool getBytes(void *dest, size_t size) {
         std::lock_guard<std::mutex> lock(mutex);
         return RandomGeneratorProvider::getBytes(dest, size);
      }

   private:
      std::mutex mutex;
   };
}

PLATFORM_PUBLIC_API
Couchbase::RandomGenerator::RandomGenerator(bool s) : shared(s) {
   if (shared) {
      static SharedRandomGeneratorProvider singleton_provider;
      provider = &singleton_provider;
   } else {
      provider = new RandomGeneratorProvider();
   }
}

PLATFORM_PUBLIC_API
Couchbase::RandomGenerator::~RandomGenerator() {
   if (!shared) {
      delete provider;
   }
}

PLATFORM_PUBLIC_API
uint64_t Couchbase::RandomGenerator::next(void) {
   uint64_t ret;
   if (provider->getBytes(&ret, sizeof(ret))) {
      return ret;
   }

   return std::chrono::steady_clock::now().time_since_epoch().count();
}

PLATFORM_PUBLIC_API
bool Couchbase::RandomGenerator::getBytes(void *dest, size_t size) {
   return provider->getBytes(dest, size);
}

PLATFORM_PUBLIC_API
const Couchbase::RandomGeneratorProvider *Couchbase::RandomGenerator::getProvider(void) const {
   return provider;
}
