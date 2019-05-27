#include "PreHeader.hpp"
#include "LoggerWrapper.hpp"

using namespace ECSTest;

#define SEND_MESSAGE(Level) \
	if (_logger == nullptr) \
	{ \
		return; \
	} \
	va_list args; \
	va_start(args, format); \
	_logger->Message(Level, _name, format, args); \
	va_end(args)

LoggerWrapper::LoggerWrapper(decltype(_logger) logger, string_view name) : _logger(logger), _name(name)
{
}

void LoggerWrapper::Message(LogLevels::LogLevel level, PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(level);
}

void LoggerWrapper::Info(PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(LogLevels::Info);
}

void LoggerWrapper::Error(PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(LogLevels::Error);
}

void LoggerWrapper::Warning(PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(LogLevels::Warning);
}

void LoggerWrapper::Critical(PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(LogLevels::Critical);
}

void LoggerWrapper::Debug(PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(LogLevels::Debug);
}

void LoggerWrapper::Attention(PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(LogLevels::Attention);
}

void LoggerWrapper::Other(PRINTF_VERIFY_FRONT const char *format, ...)
{
	SEND_MESSAGE(LogLevels::Other);
}