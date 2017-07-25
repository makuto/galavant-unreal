// The MIT License (MIT) Copyright (c) 2017 Macoy Madson

#include "GalavantUnreal.h"

#include "GalavantUnrealLog.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

// @Callback: CustomLogOutputFunc
void UnrealLogOutput(const gv::Logging::Record& record)
{
	bool IsWarning = (record.severity == gv::Logging::Severity::warning);
	bool IsError = (record.severity <= gv::Logging::Severity::error);

	static char funcNameBuffer[256];
	gv::Logging::FormatFuncName(funcNameBuffer, record.Function, sizeof(funcNameBuffer));

	static char buffer[2048];
	snprintf(buffer, sizeof(buffer), "%s():%lu: %s", funcNameBuffer,
	         (unsigned long)record.Line, record.OutBuffer);

	if (IsError)
	{
		// The UTF8_TO_TCHAR() is very important. Don't move it out of the macro without first
		// making sure its return has a sensible lifetime
		UE_LOG(LogGalavantUnreal, Error, TEXT("%s"), UTF8_TO_TCHAR(buffer));
	}
	else if (IsWarning)
	{
		UE_LOG(LogGalavantUnreal, Warning, TEXT("%s"), UTF8_TO_TCHAR(buffer));
	}
	else
	{
		UE_LOG(LogGalavantUnreal, Log, TEXT("%s"), UTF8_TO_TCHAR(buffer));
	}
}