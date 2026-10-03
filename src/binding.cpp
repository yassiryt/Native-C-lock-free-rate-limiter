#include <napi.h>
#include "limiter.hpp"

bool	g_initialized = false;

Napi::Value	InitSharedMemory(const Napi::CallbackInfo& info)
{
	Napi::Env		env = info.Env();
	Napi::Object	config;
	int32_t		max_tokens = 10;
	uint64_t	refill_ms = 1000;
	const char*	err_msg = nullptr;

	if (info.Length() > 0 && info[0].IsObject())
	{
		config = info[0].As<Napi::Object>();
		if (config.Has("maxTokens"))
			max_tokens = config.Get("maxTokens").As<Napi::Number>().Int32Value();
		if (config.Has("windowMs"))
			refill_ms = config.Get("windowMs").As<Napi::Number>().Int64Value();
	}

	if (g_initialized)
	{
		Napi::Error::New(env, "Limiter already initialized").ThrowAsJavaScriptException();
		return env.Null();
	}

	if (!init_limiter(max_tokens, refill_ms, &err_msg))
	{
		Napi::Error::New(env, err_msg).ThrowAsJavaScriptException();
		return env.Null();
	}

	g_initialized = true;
	return Napi::Boolean::New(env, true);
}

Napi::Boolean	ConsumeTokenFast(const Napi::CallbackInfo& info)
{
	Napi::Env		env = info.Env();
	std::string		ip;

	if (info.Length() < 1 || !info[0].IsString())
		return Napi::Boolean::New(env, false);

	ip = info[0].As<Napi::String>().Utf8Value();
	return Napi::Boolean::New(env, consume_token(ip.c_str(), max_tokens, refill_ms));
}

Napi::Value	Cleanup(const Napi::CallbackInfo& info)
{
	cleanup_limiter();
	return info.Env().Undefined();
}

Napi::Object	Register(Napi::Env env, Napi::Object exports)
{
	exports.Set("initSharedMemory", Napi::Function::New(env, InitSharedMemory));
	exports.Set("consumeTokenFast", Napi::Function::New(env, ConsumeTokenFast));
	exports.Set("cleanup", Napi::Function::New(env, Cleanup));
	return exports;
}

NODE_API_MODULE(rate_limiter, Register)