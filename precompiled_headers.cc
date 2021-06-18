/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

// This file is required to build the precompiled header object libraries
// (platform_pch et. al.) because object libraries need some source to compile.

// To silence linker errors when we attempt to link an empty file,
// define a totally unused symbol:
//     ranlib: file: platform/libplatform.a(precompiled_headers.cc.o) has no
//     symbols
void platform_precompiled_headers_dummy_symbol() {
    return;
}
