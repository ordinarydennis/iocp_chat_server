#include "Logger.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

namespace JCommon
{
	std::shared_ptr<spdlog::logger> Logger::mInfoFileLogger = 
		spdlog::rotating_logger_mt("info_logger", "logs/info-log.txt", MAX_FILE_SIZE, MAX_FILE_COUNT);

	std::shared_ptr<spdlog::logger> Logger::mErrorFileLogger = 
		spdlog::rotating_logger_mt("error_logger", "logs/error-log.txt", MAX_FILE_SIZE, MAX_FILE_COUNT);

	void Logger::Info(const char* fmt, ...)
	{
		char msg[MSG_SIZE] = { 0, };

		va_list args;
		va_start(args, fmt);
		vsnprintf(msg, MSG_SIZE, fmt, args);
		va_end(args);

		spdlog::info(msg);
		mInfoFileLogger->info(msg);
	}

	void Logger::Error(const char* fmt, ...)
	{
		char msg[MSG_SIZE] = { 0, };
		
		va_list args;
		va_start(args, fmt);
		vsnprintf(msg, MSG_SIZE, fmt, args);
		va_end(args);

		spdlog::error(msg);
		mErrorFileLogger->error(msg);
	}
}
