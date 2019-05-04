#pragma once

namespace ECSTest
{
	class LoggerWrapper
	{
        friend class SystemsManagerMT;
        friend class SystemsManagerST;

		Logger<string_view, true> *_logger{};
		string_view _name{};

	public:
		LoggerWrapper(decltype(_logger) logger, string_view name);
		void Message(LogLevels::LogLevel level, const char *format, ...);
	};
}