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
		void Message(LogLevels::LogLevel level, PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(3, 4);
		void Info(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
		void Error(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
		void Warning(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
		void Critical(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
		void Debug(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
		void Attention(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
		void Other(PRINTF_VERIFY_FRONT const char *format, ...) PRINTF_VERIFY_BACK(2, 3);
	};
}