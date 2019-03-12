/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
*     Copyright 2014 Couchbase, Inc.
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
#ifndef STRINGS_H
#define STRINGS_H

#include <stdarg.h>

/*
* We have a fair amount of use of this file in our code base.
* Let's just make a dummy file to aviod a lot of #ifdefs
*/
#ifndef strcasecmp
#define strcasecmp(a, b) _stricmp(a, b)
#endif

#ifndef snprintf
#define snprintf _snprintf
#endif

#endif
