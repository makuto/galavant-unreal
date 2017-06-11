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
		bool IsWarning = (record.getSeverity() == plog::warning);
		bool IsError = (record.getSeverity() <= plog::error);

		if (IsError)
		{
			// The UTF8_TO_TCHAR() is very important. Don't move it out of the macro without first
			// making sure its return has a sensible lifetime
			UE_LOG(LogGalavantUnreal, Error, TEXT("%s"),
			       UTF8_TO_TCHAR(Formatter::format(record).c_str()));
		}
		else if (IsWarning)
		{
			UE_LOG(LogGalavantUnreal, Warning, TEXT("%s"),
			       UTF8_TO_TCHAR(Formatter::format(record).c_str()));
		}
		else
		{
			UE_LOG(LogGalavantUnreal, Log, TEXT("%s"),
			       UTF8_TO_TCHAR(Formatter::format(record).c_str()));
		}
	}
};