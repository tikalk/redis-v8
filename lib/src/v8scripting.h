/*
 * Copyright (c) 2013, Arseniy Pavlenko <h0x91b@gmail.com>
 * All rights reserved.
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __V8SCRIPTING_H__
#define __V8SCRIPTING_H__

/*V8 scripting defines*/
#include <stdio.h>
#include <v8.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

extern "C" {
	#include "redis.h"
}

extern void* (*zmallocPtr)(size_t);
extern void (*zfreePtr)(void*);

using namespace v8;

extern char lastError[4096];
extern char *redisReply;
extern v8::Isolate* isolate;
extern v8::Persistent<v8::Context> persistent_v8_context;
extern char* run_js_returnbuf;
extern int run_js_returnbuf_len;

struct RUN_JS_RETURN {
	char *json;
	int len;
};

v8::Handle<v8::Value> parse_response();
RUN_JS_RETURN *run_js(char *code, bool async_call=false);
RUN_JS_RETURN *call_js(redisClient *c);
#endif