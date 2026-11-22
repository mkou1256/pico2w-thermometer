#ifndef LOGGER__H
#define LOGGER__H

#define LOG_AT(lvl, ...) \
    do { \
        if (lvl >= Logger::log_level()) Logger::log(lvl, __LINE__, __VA_ARGS__); \
    } while(0)
#define LOG_ERROR(...)   LOG_AT(LogLvl::ERROR, __VA_ARGS__)
#define LOG_WARN(...)   LOG_AT(LogLvl::WARNING, __VA_ARGS__)
#define LOG_INFO(...)   LOG_AT(LogLvl::INFO, __VA_ARGS__)

#ifdef APP_DEBUG
#define LOG_DEBUG(...)  LOG_AT(LogLvl::DEBUG, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

class ComModule;

enum LogLvl {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
};

class Logger {
public:
    static void init(ComModule* comModule);
    static void log(LogLvl level, int line, const char* message, ...);
    static LogLvl log_level(); 

private:
};

#endif // LOGGER__H