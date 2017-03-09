#pragma once

#include <plog/Log.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <plog/Record.h>

template <class Formatter>
class GalavantUnrealLog : public plog::IAppender
{
public:
	virtual void write(const plog::Record& record)
	{
		// TODO: Unreal severity
		//plog::Severity severity = record.getSeverity();
		//bool IsError = (severity >= plog::fatal && severity <= plog::warning);
		//ELogVerbosity verbosity = (IsError ? ELogVerbosity::Error : ELogVerbosity::Log);

		// The UTF8_TO_TCHAR() is very important. Don't move it out of the macro without first
		// making sure its return has a sensible lifetime
		UE_LOG(LogGalavantUnreal, Log, TEXT("%s"),
		       UTF8_TO_TCHAR(Formatter::format(record).c_str()));
	}
};