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

#include "v8core_js.h"
#include "v8scripting.h"

using namespace v8;

Persistent<Context> persistent_v8_context;
v8::Isolate* isolate;

const char* ToCString(const v8::String::Utf8Value& value);
void *single_thread_function_for_slow_run_js(void *param);
void *setTimeoutExec(void *param);
v8::Handle<v8::Value> parse_response();
char *js_dir = NULL;
char *js_flags = NULL;
int js_code_id = 0;
pthread_t thread_id_for_single_thread_check;
pthread_t thread_id_for_setTimeoutExec;
int js_timeout = 15;
int js_slow = 250;
char *last_js_run = NULL;

redisClient *client=NULL;

int scriptStart = 0;
int timeoutScriptStart = 0;

unsigned int GetTickCount(void) 
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + (tv.tv_usec/1000);
}

void getLastError(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(v8::String::New(lastError));
}

void raw_get(const v8::FunctionCallbackInfo<v8::Value>& args) {
	redisClient *c = client;
	v8::String::Utf8Value strkey(args[0]);
	robj *key = createStringObject((char*)*strkey,strkey.length());
	robj *reply = lookupKeyRead(c->db,key);
	decrRefCount(key);
	if(reply == NULL || reply->type != REDIS_STRING){
		//printf("reply is NULL or not string\n");
		args.GetReturnValue().Set(v8::Null());
		return;
	}
	//printf("reply is %s\n",reply->ptr);
	v8::Local<v8::String> v8reply = v8::String::New((const char *)reply->ptr);
	//printf("return to v8\n");
	args.GetReturnValue().Set(v8reply);
}

void raw_set(const v8::FunctionCallbackInfo<v8::Value>& args) {
	redisClient *c = client;
	v8::String::Utf8Value strkey(args[0]);
	v8::String::Utf8Value strval(args[1]);
	robj *key = createStringObject((char*)*strkey,strkey.length());
	robj *val = createStringObject((char*)*strval,strval.length());
	setKey(c->db,key,val);
	notifyKeyspaceEvent(REDIS_NOTIFY_STRING,(char*)"set",key,c->db->id);
	decrRefCount(key);
	decrRefCount(val);
	args.GetReturnValue().Set(v8::Boolean::New(true));
}

void raw_incrby(const v8::FunctionCallbackInfo<v8::Value>& args) {
	redisClient *c = client;
	long long value, oldvalue, incr;
	robj *newvalue, *key, *reply;
	v8::String::Utf8Value strkey(args[0]);
	Local<Integer> i = Local<Integer>::Cast(args[1]);
	incr = (long long)(i->IntegerValue());
	key = createStringObject((char*)*strkey,strkey.length());
	reply = lookupKeyRead(c->db,key);
	
	if (reply != NULL && checkType(c,reply,REDIS_STRING)){
		memset(lastError,0,4096);
		strcpy(lastError,"-value is not integer");
		printf("lastError set to '%s'\n",lastError);
		decrRefCount(key);
		args.GetReturnValue().Set(v8::Boolean::New(false));
		return;
	}
    if (getLongLongFromObjectOrReply(c,reply,&value,NULL) != REDIS_OK) {
		memset(lastError,0,4096);
		strcpy(lastError,"-getLongLongFromObjectOrReply failed");
		printf("lastError set to '%s'\n",lastError);
		args.GetReturnValue().Set(v8::Boolean::New(false));
		return;
	}
	
	oldvalue = value;
    if (
		(incr < 0 && oldvalue < 0 && incr < (LLONG_MIN-oldvalue))
		|| (incr > 0 && oldvalue > 0 && incr > (LLONG_MAX-oldvalue))
	) 
	{
		memset(lastError,0,4096);
		strcpy(lastError,"-increment or decrement would overflow");
		printf("lastError set to '%s'\n",lastError);
		decrRefCount(key);
		args.GetReturnValue().Set(v8::Boolean::New(false));
		return;
	}
	value += incr;
	newvalue = createStringObjectFromLongLong(value);
	if (reply){
		dbOverwrite(c->db,key,newvalue);
	}
	else{
		dbAdd(c->db,key,newvalue);
	}
	signalModifiedKey(c->db,key);
	notifyKeyspaceEvent(REDIS_NOTIFY_STRING,(char*)"incrby",key,c->db->id);

	//printf("reply is %s\n",reply->ptr);
	//v8 max integer is 2^53 (+9007199254740992) (-9007199254740992)
	if( value > 9007199254740992 || value < -9007199254740992){
		char buf[20] = {0};
		sprintf(buf,"%lli",value);
		v8::Local<v8::String> v8reply = v8::String::New(buf);
		decrRefCount(key);
		decrRefCount(newvalue);
		args.GetReturnValue().Set(v8reply);
		return;
	}
	v8::Local<v8::Number> v8reply = v8::Number::New(value);
	decrRefCount(key);
	args.GetReturnValue().Set(v8reply);
}

void run(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolateScope(isolate);
	HandleScope handle_scope(isolate);
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, persistent_v8_context);
	int argc = args.Length();
	redisCommand *cmd;
	robj **argv;
	redisClient *c = client;
	sds reply;
	
	argv = (robj**)zmalloc(sizeof(robj*)*argc);
	
	for (int i = 0; i < args.Length(); i++) {
		HandleScope handle_scope(isolate);
		v8::String::Utf8Value str(args[i]);
		argv[i] = createStringObject((char*)*str,str.length());
	}
	
	/* Setup our fake client for command execution */
	c->argv = argv;
	c->argc = argc;
	
	/* Command lookup */
	cmd = lookupCommandByCString((sds)argv[0]->ptr);
	if(!cmd){
		printf("no cmd '%s'!!!\n",(char*)argv[0]->ptr);
		args.GetReturnValue().Set(v8::Undefined());
		return;
	}
	/* Run the command */
	c->cmd = cmd;
	c->cmd->proc(c); //raw call, without redis stats log
	//call(c,REDIS_CALL_STATS);
	reply = sdsempty();
	if (c->bufpos) {
		reply = sdscatlen(reply,c->buf,c->bufpos);
		c->bufpos = 0;
	}
	
	while(listLength(c->reply)) {
		robj *o = (robj*)listNodeValue(listFirst(c->reply));
		reply = sdscatlen(reply,o->ptr,strlen((const char*)o->ptr));
		listDelNode(c->reply,listFirst(c->reply));
	}
	
	redisReply = reply;
	v8::Handle<v8::Value> ret_value= parse_response();
	v8::Local<v8::String> v8reply = v8::String::New(reply);
	
	sdsfree(reply);
	c->reply_bytes = 0;
	
	for (int j = 0; j < c->argc; j++)
		decrRefCount(c->argv[j]);
	zfree(c->argv);
	
	args.GetReturnValue().Set(ret_value);
}

char *file_get_contents(char *filename)
{
	FILE* f = fopen(filename, "r");
	if(!f) return NULL;
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	char* content = (char*)zmalloc(size+1);
	memset(content,0,size);
	rewind(f);
	fread(content, sizeof(char), size, f);
	content[size] = '\0';
	return content;
}

const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

void redis_log(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if(args.Length()>=2){
		Local<Integer> i = Local<Integer>::Cast(args[0]);
		int log_level = (int)(i->Int32Value());
		v8::String::Utf8Value str(args[1]);
		const char* cstr = ToCString(str);
		redisLogRaw(log_level, (char*)cstr);
	}
}

void initV8(){
	if(js_flags){
		v8::V8::SetFlagsFromString(
			js_flags,
			strlen(js_flags)
		);
		v8::Debug::EnableAgent("redis-v8", 5858, false);
	}
	
	pthread_create(&thread_id_for_single_thread_check, NULL, single_thread_function_for_slow_run_js, (void*)NULL);
	
	isolate = v8::Isolate::GetCurrent();
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolateScope(isolate);
	HandleScope handle_scope(isolate);
	
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	v8::Handle<v8::ObjectTemplate> redis = v8::ObjectTemplate::New();
	redis->Set(v8::String::New("__run"), v8::FunctionTemplate::New(run),ReadOnly);
	redis->Set(v8::String::New("__get"), v8::FunctionTemplate::New(raw_get),ReadOnly);
	redis->Set(v8::String::New("__set"), v8::FunctionTemplate::New(raw_set),ReadOnly);
	redis->Set(v8::String::New("__incrby"), v8::FunctionTemplate::New(raw_incrby),ReadOnly);
	redis->Set(v8::String::New("__log"), v8::FunctionTemplate::New(redis_log),ReadOnly);
	redis->Set(v8::String::New("getLastError"), v8::FunctionTemplate::New(getLastError),ReadOnly);
	global->Set(v8::String::New("redis"), redis);
	
	// Create a new context.
	v8::Handle<v8::Context> v8_context = v8::Context::New(isolate,NULL,global);
	
	persistent_v8_context.Reset(isolate, v8_context);
	
	// Enter the created context for compiling and running
	v8::Context::Scope context_scope(v8_context);
	
	v8::Handle<v8::String> source = v8::String::New((const char*)v8core_js);
	v8::Handle<v8::Script> script = v8::Script::New(source, v8::String::New("v8core.js"));
	v8::Handle<v8::Value> result = script->Run();
}

void load_user_script(char *file){
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolateScope(isolate);
	HandleScope handle_scope(isolate);
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, persistent_v8_context);

	v8::Context::Scope context_scope(v8_context);
	char* core = file_get_contents(file);
	v8::Handle<v8::String> source = v8::String::New(core);
	v8::TryCatch trycatch;
	v8::Handle<v8::Script> script = v8::Script::New(source, v8::String::New(file));
	if(script.IsEmpty()){
		Handle<Value> exception = trycatch.Exception();
		String::AsciiValue exception_str(exception);
		printf("V8 Exception: %s\n", *exception_str);
		char *errBuf = (char*)zmalloc(4096); //TODO: calc size
		memset(errBuf,0,4096);
		sprintf(errBuf,"-Compile error: \"%s\"",*exception_str);
		printf("errBuf is '%s'\n",errBuf);
		return;
	}
	v8::Handle<v8::Value> result = script->Run();
	if (result.IsEmpty()) {  
		Handle<Value> exception = trycatch.Exception();
		String::AsciiValue exception_str(exception);
		printf("Exception: %s\n", *exception_str);
		char *errBuf = (char*)zmalloc(4096); //TODO: calc size
		memset(errBuf,0,4096);
		sprintf(errBuf,"-Exception error: \"%s\"",*exception_str);
		return;
	}
	zfree(core);
}

void load_user_scripts_from_folder(char *folder){
	DIR *dp;
	struct dirent *dirp;
	unsigned char isFolder =0x4;
	int len = 0;
	if((dp  = opendir(folder)) != NULL) {
		while ((dirp = readdir(dp)) != NULL) {
			//files.push_back(string(dirp->d_name));
			if(strcmp(".", dirp->d_name) && strcmp("..", dirp->d_name)){
				len = strlen (dirp->d_name);
				if(dirp->d_type == isFolder){
					char subfolder[1024] = {0};
					sprintf(subfolder,"%s%s/",folder,dirp->d_name);
					load_user_scripts_from_folder(subfolder);
				}
				else if(strcmp (".js", &(dirp->d_name[len - 3])) == 0){
					char file[1024] = {0};
					sprintf(file,"%s%s",folder,dirp->d_name);
					redisLogRaw(REDIS_NOTICE,file);
					load_user_script(file);
				}
			}
		}
		closedir(dp);
	} else {
		redisLogRaw(REDIS_NOTICE, (char*)"js-dir from config - not found");
	}
}

struct ThreadJSClientAndCode {
	redisClient *c;
	char *code;
};

void *setTimeoutExec(void *param)
{
	while(1){
		usleep(50000); //50ms
		v8::Locker locker(isolate);
		v8::Isolate::Scope isolateScope(isolate);
		HandleScope handle_scope(isolate);
		v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, persistent_v8_context);
		Locker v8Locker(isolate);
		v8::Context::Scope context_scope(v8_context);
		v8::Handle<v8::String> source = v8::String::New("redis._runtimeouts()");
		v8::TryCatch trycatch;
		v8::Handle<v8::Script> script = v8::Script::Compile(source);
		if(script.IsEmpty()){
			Handle<Value> exception = trycatch.Exception();
			String::AsciiValue exception_str(exception);
			printf("V8 Exception: %s\n", *exception_str);
			char *errBuf = (char*)zmalloc(4096); //TODO: calc size
			memset(errBuf,0,4096);
			sprintf(errBuf,"-Compile error: \"%s\"",*exception_str);
			printf("errBuf is '%s'\n",errBuf);
			continue;
		}
		timeoutScriptStart = GetTickCount();
		v8::Handle<v8::Value> result = script->Run();
		timeoutScriptStart = 0;
		if (result.IsEmpty()) {  
			Handle<Value> exception = trycatch.Exception();
			String::AsciiValue exception_str(exception);
			printf("Exception: %s\n", *exception_str);
			char *errBuf = (char*)zmalloc(4096); //TODO: calc size
			memset(errBuf,0,4096);
			sprintf(errBuf,"-Exception error: \"%s\"",*exception_str);
			continue;
		}
	}
	return 0;
}

void *single_thread_function_for_slow_run_js(void *param)
{
	bool slow_report = false;
	while(1){
		usleep(100000); //100ms
		unsigned int dt = GetTickCount() - scriptStart;
		if(scriptStart != 0 && last_js_run!=NULL && dt > js_slow){
			if(!slow_report){
				slow_report = true;
				printf("run_js running more than %ims, log function\n",js_slow);
				redisLogRaw(REDIS_NOTICE, (char*)"JS slow function:");
				redisLogRaw(REDIS_NOTICE, (char*)last_js_run);
			}
		}
		else
			slow_report = false;
		
		if(scriptStart != 0 && last_js_run!=NULL && dt > js_timeout*1000){
			printf("run_js running more than %i sec, kill it\n",js_timeout);
			redisLogRaw(REDIS_NOTICE, (char*)"JS to slow function, kill it:");
			redisLogRaw(REDIS_NOTICE, (char*)last_js_run);
			v8::V8::TerminateExecution();
			scriptStart = 0;
		}
		
		unsigned int dtt = GetTickCount() - timeoutScriptStart;
		if(timeoutScriptStart != 0 && dtt > js_timeout*1000){
			printf("some of timeout/interval runned for %i sec, kill it\n",js_timeout);
			redisLogRaw(REDIS_NOTICE, (char*)"some of timeouts/intervals works to long, kill last one.");
			v8::V8::TerminateExecution();
			run_js((char*)"clearInterval(redis._last_interval_id)");
			timeoutScriptStart = 0;
		}
	}
	return 0;
}

extern "C"
{
	void v8_exec(redisClient *c,char* code){
		//printf("v8_exec %s\n",code);
		scriptStart = GetTickCount();
		last_js_run = code;
		RUN_JS_RETURN * ret = run_js(code,false);
		last_js_run = NULL;
		scriptStart = 0;
		if(ret->json && ret->json[0]=='-'){
			printf("run_js return error %s\n",ret->json);
			addReplyError(c,ret->json);
			if(ret->json!=run_js_returnbuf) zfree(ret->json);
			return;
		}
		robj *obj = createStringObject(ret->json,ret->len);
		addReplyBulk(c,obj);
		decrRefCount(obj);
	}
	
	void v8_exec_async(redisClient *c,char* code){
		//printf("v8_exec %s\n",code);
		scriptStart = GetTickCount();
		last_js_run = code;
		RUN_JS_RETURN * ret = run_js(code,true);
		last_js_run = NULL;
		scriptStart = 0;
		if(ret->json && ret->json[0]=='-'){
			printf("run_js return error %s\n",ret->json);
			addReplyError(c,ret->json);
			if(ret->json!=run_js_returnbuf) zfree(ret->json);
			return;
		}
		robj *obj = createStringObject(ret->json,ret->len);
		addReplyBulk(c,obj);
		decrRefCount(obj);
	}
	
	void v8_exec_call(redisClient *c){
		if(c->argc<2){
			addReplyError(c,(char*)"-Wrong number of arguments, must be at least 2");
			return;
		}
		//printf("v8_exec_call args %i\n",c->argc);
		scriptStart = GetTickCount();
		last_js_run = (char*)c->argv[1]->ptr;
		RUN_JS_RETURN * ret = call_js(c);
		last_js_run = NULL;
		scriptStart = 0;
		if(ret->json && ret->json[0]=='-'){
			printf("call_js return error %s\n",ret->json);
			addReplyError(c,ret->json);
			if(ret->json!=run_js_returnbuf) zfree(ret->json);
			return;
		}
		robj *obj = createStringObject(ret->json,ret->len);
		addReplyBulk(c,obj);
		decrRefCount(obj);
	}
	
	void v8_reload(redisClient *c){
		v8::V8::TerminateExecution();
		persistent_v8_context.Dispose();
		pthread_cancel(thread_id_for_single_thread_check);
		initV8();
		redisLogRaw(REDIS_NOTICE, (char*)"V8 core loaded");
		load_user_scripts_from_folder(js_dir);
		redisLogRaw(REDIS_NOTICE, (char*)"V8 user script loaded");
		addReply(c,createObject(REDIS_STRING,sdsnew("+V8 Reload complete\r\n")));
		pthread_create(&thread_id_for_setTimeoutExec, NULL, setTimeoutExec, (void*)NULL);
	}
	
	void v8setup()
	{
		redisLogRaw(REDIS_NOTICE, (char*)"Making redisClient\n");
		client = createClient(-1);
		client->flags |= REDIS_LUA_CLIENT;
		
		initV8();
		
		if(js_dir==NULL){
			js_dir = (char*)zmalloc(1024);
			strcpy(js_dir,"./js/");
		}

		redisLogRaw(REDIS_NOTICE, (char*)"V8 core loaded");
		load_user_scripts_from_folder(js_dir);
		redisLogRaw(REDIS_NOTICE, (char*)"V8 user script loaded");
		
		pthread_create(&thread_id_for_setTimeoutExec, NULL, setTimeoutExec, (void*)NULL);
	}
		
	void config_js_dir(char *_js_dir){
		printf("config_js_dir %s\n",_js_dir);
		if(js_dir) zfree(js_dir);
		js_dir = (char*)zmalloc(1024);
		strcpy(js_dir,_js_dir);
	}
	
	void config_js_flags(char *_js_flags){
		printf("config_js_flags %s\n",_js_flags);
		if(js_flags) zfree(js_flags);
		js_flags = (char*)zmalloc(1024);
		strcpy(js_flags,_js_flags);
	}
	
	void config_js_timeout(int timeout){
		printf("config_js_timeout %i\n",timeout);
		js_timeout = timeout;
	}
	
	void config_js_slow(int slow){
		printf("config_js_slow %i\n",slow);
		js_slow = slow;
	}
	
	char *config_get_js_dir(){
		return js_dir;
	}
	
	char *config_get_js_flags(){
		return js_flags;
	}
	
	int config_get_js_timeout(){
		return js_timeout;
	}
	
	int config_get_js_slow(){
		return js_slow;
	}
}
