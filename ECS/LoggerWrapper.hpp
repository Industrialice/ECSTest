#pragma once

namespace ECSTest
{
	class LoggerWrapper
	{
		Logger<string_view, true> *_logger{};
		string_view _name{};

	public:
		LoggerWrapper() = default;
		LoggerWrapper(decltype(_logger) logger, string_view name);
		void Message(LogLevels::LogLevel level, PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
		void Info(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(1, 2);
		void Error(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(1, 2);
		void Warning(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(1, 2);
		void Critical(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(1, 2);
		void Debug(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(1, 2);
		void Attention(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(1, 2);
		void Other(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(1, 2);
	};
}