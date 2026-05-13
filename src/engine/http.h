#ifndef ENGINE_HTTP_H
#define ENGINE_HTTP_H

#include "kernel.h"

#include <chrono>
#include <memory>

class IHttpRequest
{
};

class IHttp : public IInterface
{
	MACRO_INTERFACE("http")

public:
	virtual void Run(std::shared_ptr<IHttpRequest> pRequest) = 0;
};

class IEngineHttp : public IHttp
{
	MACRO_INTERFACE("enginehttp")

public:
	virtual bool Init(std::chrono::milliseconds ShutdownDelay) = 0;
	void Shutdown() override = 0;
};

IEngineHttp *CreateEngineHttp();

#endif
