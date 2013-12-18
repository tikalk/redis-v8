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

#include "v8scripting.h"

using namespace v8;

char *redisReply = NULL;
char lastError[4096] = {0};
char bufForString[4096] = {0};

v8::Handle<v8::Value> parse_string(char *replyPtr){
	//printf("parse_line_ok replyPtr[0]='%c' string length:%i\n",replyPtr[0],atoi(replyPtr));
	int strlength = atoi(replyPtr);
	bool special_minus_one = false;
	if(strlength==-1){
		strlength = 0;
		special_minus_one = true;
	}
	int len = strstr(replyPtr,"\r\n")-replyPtr;
	if(special_minus_one) len-=2;
	replyPtr+=len+2;
	if(strlength<4096){
		memcpy(bufForString,replyPtr,strlength);
		replyPtr+=strlength+2;
		bufForString[strlength]='\0';
		redisReply = replyPtr;
		if(special_minus_one) return v8::Null();
		v8::Local<v8::String> ret = v8::String::New(bufForString,strlength);
		return ret;
	}
	char *buff= (char*)zmalloc(strlength+1);
	memcpy(buff,replyPtr,strlength);
	replyPtr+=strlength+2;
	buff[strlength]='\0';
	if(strlen(bufForString)!=strlength){
		//binary data
		v8::Local<v8::Array> ret = v8::Array::New(strlength);
		for(int i=0;i<strlength;i++){
			ret->Set(v8::Number::New(i), v8::Number::New((unsigned char)bufForString[i]));
		}
		zfree(buff);
		return ret;
	}
	//printf("line is '%s'\n",buff);
	v8::Local<v8::String> ret = v8::String::New(buff);
	zfree(buff);
	redisReply = replyPtr;
	if(special_minus_one) return v8::Null();
	return ret;
}

v8::Handle<v8::Value> parse_error(char *replyPtr){
	int len = strstr(replyPtr,"\r\n")-replyPtr;
	memset(lastError,0,4096);
	strncpy(lastError,replyPtr,len);
	replyPtr+=len+2;
	redisReply = replyPtr;
	printf("lastError set to '%s'\n",lastError);
	return v8::Boolean::New(false);
}

v8::Handle<v8::Value> parse_bulk(char *replyPtr){
	int arr_length = atoi(replyPtr);
	int len = strstr(replyPtr,"\r\n")-replyPtr;
	replyPtr+=len+2;
	redisReply = replyPtr;
	v8::Local<v8::Array> ret = v8::Array::New(arr_length);
	for(int i=0;i<arr_length;i++){
		ret->Set(v8::Number::New(i), parse_response());
	}
	return ret;
}


v8::Handle<v8::Value> parse_response(){
	char *replyPtr = redisReply;
	long long lvalue = 0;
	//printf("replyPtr[0]='%c' reply='%s'\n",replyPtr[0],replyPtr);
	switch(replyPtr[0]){
		case '+':
			return v8::Boolean::New(true);
		case '-':
			return parse_error(++replyPtr);
		case ':':
			lvalue = atoll(++replyPtr);
			if(lvalue > 9007199254740992 || lvalue < -9007199254740992){
				char buf[20] = {0};
				sprintf(buf,"%lli",lvalue);
				v8::Local<v8::String> v8reply = v8::String::New(buf);
				return v8reply;
			}
			return v8::Integer::New(lvalue);
		case '$':
			return parse_string(++replyPtr);
		case '*':
			return parse_bulk(++replyPtr);
		default:
			printf("cant parse reply %s\n",replyPtr);
	}
	return v8::Undefined();
}
