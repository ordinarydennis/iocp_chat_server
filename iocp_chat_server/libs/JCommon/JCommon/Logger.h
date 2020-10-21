#pragma once
#include "spdlog/spdlog.h"
#include <memory>

namespace JCommon
{
	class Logger
	{
	public:
		Logger() = default;

		~Logger() = default;

		static void		Info(const char* fmt, ...);

		static void		Error(const char* fmt, ...);

	private:
		static const size_t MSG_SIZE = 256;
		static const size_t MAX_FILE_SIZE = 1048576 * 5;
		static const size_t MAX_FILE_COUNT = 100;
		static std::shared_ptr<spdlog::logger> mInfoFileLogger;
		static std::shared_ptr<spdlog::logger> mErrorFileLogger;
	};
}


