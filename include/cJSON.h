/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
/*
 *     Copyright 2016 Couchbase, Inc
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

#pragma once

#include <platform/visibility.h>
#include <stdbool.h>

#ifdef BUILDING_CJSON
#define CJSON_PUBLIC_API EXPORT_SYMBOL
#else
#define CJSON_PUBLIC_API IMPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* cJSON Types: */
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6

#define cJSON_IsReference 256

/* The cJSON structure: */
typedef struct cJSON {
        struct cJSON *next,*prev; /* next/prev allow you to walk
                                     array/object
                                     chains. Alternatively, use
                                     GetArraySize/GetArrayItem/GetObjectItem */
        struct cJSON *child; /* An array or object item will have a
                                child pointer pointing to a chain of
                                the items in the array/object. */

        int type; /* The type of the item, as above. */

        char *valuestring; /* The item's string, if type==cJSON_String */
        int valueint; /* The item's number, if type==cJSON_Number */
        double valuedouble; /* The item's number, if type==cJSON_Number */

        char *string; /* The item's name string, if this item is the
                         child of, or is in the list of subitems of an
                         object. */
} cJSON;

/* Supply a block of JSON, and this returns a cJSON object you can
   interrogate. Call cJSON_Delete when finished. */
CJSON_PUBLIC_API
extern cJSON *cJSON_Parse(const char *value);
/* Render a cJSON entity to text for transfer/storage. Free the char*
   when finished. */
CJSON_PUBLIC_API
extern char  *cJSON_Print(const cJSON *item);
/* Render a cJSON entity to text for transfer/storage without any
   formatting. Free the char* when finished. */
CJSON_PUBLIC_API
extern char  *cJSON_PrintUnformatted(const cJSON *item);
/* Release the memory returned by cJSON_Print and cJSON_PrintUnformatted */
CJSON_PUBLIC_API
extern void   cJSON_Free(char *ptr);
/* Delete a cJSON entity and all subentities. */
CJSON_PUBLIC_API
extern void   cJSON_Delete(cJSON *c);

/* Returns the number of items in an array (or object). */
CJSON_PUBLIC_API
extern int    cJSON_GetArraySize(cJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if
   unsuccessful. */
CJSON_PUBLIC_API
extern cJSON *cJSON_GetArrayItem(cJSON *array,int item);
/* Get item "string" from object. Case insensitive. */
CJSON_PUBLIC_API
extern cJSON *cJSON_GetObjectItem(cJSON *object,const char *string);

/* These calls create a cJSON item of the appropriate type. */
CJSON_PUBLIC_API
extern cJSON *cJSON_CreateNull(void);
CJSON_PUBLIC_API
extern cJSON *cJSON_CreateTrue(void);
CJSON_PUBLIC_API
extern cJSON *cJSON_CreateFalse(void);
CJSON_PUBLIC_API
extern cJSON *cJSON_CreateNumber(double num);
CJSON_PUBLIC_API
extern cJSON *cJSON_CreateString(const char *string);
CJSON_PUBLIC_API
extern cJSON *cJSON_CreateArray(void);
CJSON_PUBLIC_API
extern cJSON *cJSON_CreateObject(void);

/* Append item to the specified array/object. */
CJSON_PUBLIC_API
extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC_API
extern void cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item);
/* Append reference to item to the specified array/object. Use this
   when you want to add an existing cJSON to a new cJSON, but don't
   want to corrupt your existing cJSON. */
CJSON_PUBLIC_API
extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC_API
extern void cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
CJSON_PUBLIC_API
extern cJSON *cJSON_DetachItemFromArray(cJSON *array,int which);
CJSON_PUBLIC_API
extern void   cJSON_DeleteItemFromArray(cJSON *array,int which);
CJSON_PUBLIC_API
extern cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string);
CJSON_PUBLIC_API
extern void   cJSON_DeleteItemFromObject(cJSON *object,const char *string);

/* Update array items. */
CJSON_PUBLIC_API
extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
CJSON_PUBLIC_API
extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);

#define cJSON_AddNullToObject(object,name) \
        cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name) \
        cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name) \
        cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddNumberToObject(object,name,n) \
        cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object,name,s) \
        cJSON_AddItemToObject(object, name, cJSON_CreateString(s))

CJSON_PUBLIC_API
extern void cJSON_AddBoolToObject(cJSON* object, const char* string, bool value);

#ifdef __cplusplus
}
#endif
