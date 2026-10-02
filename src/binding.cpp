#include <napi.h>
#include "limiter.hpp"

int32_t		g_max_tokens = 10;
uint64_t	g_refill_ms = 1000;

Napi::Value	Init(const Napi::CallbackInfo& info)
{
	Napi::Env		env = info.Env();
	Napi::Object	config;
	const char*		err_msg = nullptr;

	if (info.Length() > 0 && info[0].IsObject())
	{
		config = info[0].As<Napi::Object>();
		if (config.Has("maxTokens"))
			g_max_tokens = config.Get("maxTokens").As<Napi::Number>().Int32Value();
		if (config.Has("windowMs"))
			g_refill_ms = config.Get("windowMs").As<Napi::Number>().Int64Value();
	}

	if (!init_limiter(g_max_tokens, g_refill_ms, &err_msg))
	{
		Napi::Error::New(env, err_msg).ThrowAsJavaScriptException();
		return env.Null();
	}

	return Napi::Boolean::New(env, true);
}

Napi::Boolean	Consume(const Napi::CallbackInfo& info)
{
	Napi::Env       env = info.Env();
	std::string     ip;

	if (info.Length() < 1 || !info[0].IsString())
		return Napi::Boolean::New(env, false);

	ip = info[0].As<Napi::String>().Utf8Value();
	bool allowed = consume_token(ip.c_str(), g_max_tokens, g_refill_ms);

	return Napi::Boolean::New(env, allowed);
}

Napi::Object	Register(Napi::Env env, Napi::Object exports)
{
	exports.Set("init", Napi::Function::New(env, Init));
	exports.Set("consume", Napi::Function::New(env, Consume));
	return exports;
}

NODE_API_MODULE(rate_limiter, Register)