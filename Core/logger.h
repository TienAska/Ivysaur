#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger
{
public:
	static void Init();

	inline static const std::shared_ptr<spdlog::logger>& OpenGL() { return s_OpenGLLogger; };
	inline static const std::shared_ptr<spdlog::logger>& Runtime() { return s_RuntimeLogger; };
private:
	static std::shared_ptr<spdlog::logger> s_OpenGLLogger;
	static std::shared_ptr<spdlog::logger> s_RuntimeLogger;

};

void Logger::Init()
{
	spdlog::set_pattern("[%H:%M:%S.%e] [%n] %^[%l]%$ %v");
	s_OpenGLLogger = spdlog::stdout_color_mt("OpenGL");
	s_OpenGLLogger->set_level(spdlog::level::trace);

	s_RuntimeLogger = spdlog::stdout_color_mt("Runtime");
	s_RuntimeLogger->set_level(spdlog::level::trace);
}

std::shared_ptr<spdlog::logger> Logger::s_OpenGLLogger;
std::shared_ptr<spdlog::logger> Logger::s_RuntimeLogger;

#define LOG_OPENGL_TRACE(...)    Logger::OpenGL()->trace(__VA_ARGS__)
#define LOG_OPENGL_DEBUG(...)    Logger::OpenGL()->debug(__VA_ARGS__)
#define LOG_OPENGL_INFO(...)     Logger::OpenGL()->info(__VA_ARGS__)
#define LOG_OPENGL_WARN(...)     Logger::OpenGL()->warn(__VA_ARGS__)
#define LOG_OPENGL_ERROR(...)    Logger::OpenGL()->error(__VA_ARGS__)
#define LOG_OPENGL_CRITICAL(...) Logger::OpenGL()->critical(__VA_ARGS__)

#define LOG_RUNTIME_TRACE(...)    Logger::Runtime()->trace(__VA_ARGS__)
#define LOG_RUNTIME_DEBUG(...)    Logger::Runtime()->debug(__VA_ARGS__)
#define LOG_RUNTIME_INFO(...)     Logger::Runtime()->info(__VA_ARGS__)
#define LOG_RUNTIME_WARN(...)     Logger::Runtime()->warn(__VA_ARGS__)
#define LOG_RUNTIME_ERROR(...)    Logger::Runtime()->error(__VA_ARGS__)
#define LOG_RUNTIME_CRITICAL(...) Logger::Runtime()->critical(__VA_ARGS__)