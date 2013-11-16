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


char *wrapcodebuf = NULL;
int wrapcodebuf_len = 4096;
char* run_js_returnbuf = NULL;
int run_js_returnbuf_len = 4096;

RUN_JS_RETURN run_js_return;

RUN_JS_RETURN *run_js(char *code, bool async_call){
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolateScope(isolate);
	HandleScope handle_scope(isolate);
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, persistent_v8_context);
	v8::Context::Scope context_scope(v8_context);
	int code_length = strlen(code);
	if(wrapcodebuf==NULL){
		wrapcodebuf_len = code_length+170;
		wrapcodebuf = (char*)zmallocPtr(wrapcodebuf_len);
	}
	if(code_length+170>wrapcodebuf_len){
		zfreePtr(wrapcodebuf);
		wrapcodebuf_len = code_length+170;
		wrapcodebuf = (char*)zmallocPtr(wrapcodebuf_len);
	}
	//memset(wrapcodebuf,0,code_length+170);
	wrapcodebuf[0] = '\0';
	if(!async_call){
		sprintf(wrapcodebuf,"inline_redis_func = function(){%s}; redis.inline_return()",code);
	} else {
		sprintf(wrapcodebuf,"(function(){ setTimeout(function(){%s},1); return '{\"ret\":true,\"cmds\":0}' })();",code);
	}
	
	//printf("%s\n",wrapcodebuf);
	
	v8::Handle<v8::String> source = v8::String::New(wrapcodebuf);
	v8::TryCatch trycatch;
	v8::Handle<v8::Script> script = v8::Script::Compile(source);
	if(script.IsEmpty()){
		Handle<Value> exception = trycatch.Exception();
		Handle<Value> stackTrace = trycatch.StackTrace();
		String::AsciiValue exception_str(exception);
		printf("V8 Exception: %s\n", *exception_str);
		String::AsciiValue stackTrace_str(stackTrace);
		if(stackTrace_str.length()>0){
			printf("stackTrace is: %s\n", *stackTrace_str);
		}
		char *errBuf = (char*)zmallocPtr(exception_str.length()+100);
		memset(errBuf,0,exception_str.length());
		sprintf(errBuf,"-Compile error: \"%s\"",*exception_str);
		printf("errBuf is '%s'\n",errBuf);
		run_js_return.json = errBuf;
		run_js_return.len = exception_str.length();
		return &run_js_return;
	}
	
	v8::Handle<v8::Value> result = script->Run();
	
	if (result.IsEmpty()) {  
		Handle<Value> exception = trycatch.Exception();
		String::AsciiValue exception_str(exception);
		Handle<Value> stackTrace = trycatch.StackTrace();
		String::AsciiValue stackTrace_str(stackTrace);
		printf("Exception: %s\n", *exception_str);
		if(stackTrace_str.length()>0)
			printf("StackTrace: %s\n", *stackTrace_str);
		char *errBuf = (char*)zmallocPtr(exception_str.length()+stackTrace_str.length()+100);
		memset(errBuf,0,exception_str.length());
		if(!strcmp(*exception_str,"null")){
			sprintf(errBuf,"-Script runs too long, Exception error: \"%s\"",*exception_str);
		}
		else {
			if(stackTrace_str.length()>0){
				sprintf(errBuf,"-Exception error: \"%s\", stackTrace: \"%s\"",*exception_str, *stackTrace_str);
			} else {
				sprintf(errBuf,"-Exception error: \"%s\"", *exception_str);
			}
		}
		run_js_return.json = errBuf;
		run_js_return.len = exception_str.length();
		return &run_js_return;
	}
	v8::String::Utf8Value ascii(result);
	int size = ascii.length();
	if(run_js_returnbuf==NULL){
		run_js_returnbuf = (char*)zmallocPtr(run_js_returnbuf_len);
		memset(run_js_returnbuf,0,run_js_returnbuf_len);
	}
	if(size>=run_js_returnbuf_len){
		zfreePtr(run_js_returnbuf);
		run_js_returnbuf_len = (((size+1)/1024)+1)*1024;
		run_js_returnbuf = (char*)zmallocPtr(run_js_returnbuf_len);
		memset(run_js_returnbuf,0,run_js_returnbuf_len);
	}
	//char *rez= (char*)zmallocPtr(size);
	//memset(run_js_returnbuf,0,size+1);
	memcpy(run_js_returnbuf,*ascii,size);
	run_js_returnbuf[size] = '\0';
	run_js_return.json = run_js_returnbuf;
	run_js_return.len = size;
	return &run_js_return;
}


RUN_JS_RETURN *call_js(redisClient *c){
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolateScope(isolate);
	HandleScope handle_scope(isolate);
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, persistent_v8_context);
	v8::Context::Scope context_scope(v8_context);
	
	Handle<v8::Object> global = v8_context->Global();
	Handle<v8::Value> value = global->Get(String::New("jscall_wrapper_function"));
	Handle<v8::Function> jscall_wrapper_function = v8::Handle<v8::Function>::Cast(value);
	
	int argc = c->argc-1;
	
	Handle<Value> *args = (Handle<Value>*)zmallocPtr(argc*sizeof(Handle<Value>));
	for (int i = 1; i <= argc; i++) { 
		args[i-1] = v8::String::New((const char*)c->argv[i]->ptr); 
	}
	
	v8::TryCatch trycatch;
	v8::Handle<v8::Value> result = jscall_wrapper_function->Call(global, argc, args);
	zfreePtr(args);
	if (result.IsEmpty()) {  
		Handle<Value> exception = trycatch.Exception();
		String::AsciiValue exception_str(exception);
		printf("Exception: %s\n", *exception_str);
		int length = exception_str.length()+100;
		char *errBuf = (char*)zmallocPtr(length);
		memset(errBuf,0,length);
		if(!strcmp(*exception_str,"null")){
			sprintf(errBuf,"-Script runs too long, Exception error: \"%s\"",*exception_str);
		}
		else {
			sprintf(errBuf,"-Exception error: \"%s\"",*exception_str);
		}
		run_js_return.json = errBuf;
		run_js_return.len = exception_str.length();
		return &run_js_return;
	}
	
	v8::String::Utf8Value ascii(result);
	int size = ascii.length();
	if(run_js_returnbuf==NULL){
		run_js_returnbuf = (char*)zmallocPtr(run_js_returnbuf_len);
		memset(run_js_returnbuf,0,run_js_returnbuf_len);
	}
	if(size>=run_js_returnbuf_len){
		zfreePtr(run_js_returnbuf);
		run_js_returnbuf_len = (((size+1)/1024)+1)*1024;
		run_js_returnbuf = (char*)zmallocPtr(run_js_returnbuf_len);
		memset(run_js_returnbuf,0,run_js_returnbuf_len);
	}
	memcpy(run_js_returnbuf,*ascii,size);
	run_js_returnbuf[size] = '\0';
	run_js_return.json = run_js_returnbuf;
	run_js_return.len = size;
	return &run_js_return;
}