#ifndef LOGGER_H
#define LOGGER_H
#ifndef LOG_LEVEL
#define LOG_LEVEL 4
#endif

#define LOG_LEVEL_FATAL    (1)
#define LOG_LEVEL_ERR      (2)
#define LOG_LEVEL_WARN     (3)
#define LOG_LEVEL_INFO     (4)
#define LOG_LEVEL_DBG      (5)
#define LOG_LEVEL_DBG2     (6)
#define LOG_LEVEL_DBG3     (7)

void log_msg_function(const char *function_name, const char *fstring, ...);

#if (LOG_LEVEL >= LOG_LEVEL_FATAL)
#define LOG_FATAL(...) log_msg_function(__func__,__VA_ARGS__)
#else
#define LOG_FATAL(...) do {} while(0)
#endif
#if (LOG_LEVEL >= LOG_LEVEL_ERR)
#define LOG_ERR(...) log_msg_function(__func__,__VA_ARGS__)
#else
#define LOG_ERR(...) do {} while(0)
#endif
#if (LOG_LEVEL >= LOG_LEVEL_WARN)
#define LOG_WARN(...) log_msg_function(__func__,__VA_ARGS__)
#else
#define LOG_WARN(...) do {} while(0)
#endif
#if (LOG_LEVEL >= LOG_LEVEL_INFO)
#define LOG_INFO(...) log_msg_function(__func__,__VA_ARGS__)
#else
#define LOG_INFO(...) do {} while(0)
#endif
#if (LOG_LEVEL >= LOG_LEVEL_DBG)
#define LOG_DBG(...) log_msg_function(__func__,__VA_ARGS__)
#else
#define LOG_DBG(...) do {} while(0)
#endif
#if (LOG_LEVEL >= LOG_LEVEL_DBG2)
#define LOG_DBG2(...) log_msg_function(__func__,__VA_ARGS__)
#else
#define LOG_DBG2(...) do {} while(0)
#endif
#if (LOG_LEVEL >= LOG_LEVEL_DBG3)
#define LOG_DBG3(...) log_msg_function(__func__,__VA_ARGS__)
#else
#define LOG_DBG3(...) do {} while(0)
#endif

#endif
