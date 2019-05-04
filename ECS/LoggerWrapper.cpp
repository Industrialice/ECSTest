#include "PreHeader.hpp"
#include "LoggerWrapper.hpp"

using namespace ECSTest;

LoggerWrapper::LoggerWrapper(decltype(_logger) logger, string_view name) : _logger(logger), _name(name)
{
}

void LoggerWrapper::Message(LogLevels::LogLevel level, const char *format, ...)
{
	if (_logger == nullptr)
	{
		return;
	}

	va_list args;
	va_start(args, format);
	_logger->Message(level, _name, format, args);
	va_end(args);
}
