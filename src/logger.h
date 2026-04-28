/* logger.h — T60: LOG_INFO / LOG_WARN / LOG_ERR makroları
 * Hem stdout'a hem game.log dosyasına yazar. */

#ifndef LOGGER_H
#define LOGGER_H

typedef enum { LOG_LEVEL_INFO = 0, LOG_LEVEL_WARN, LOG_LEVEL_ERR } LogLevel;

void InitLogger(void);
void CloseLogger(void);
void LogWrite(LogLevel level, const char *file, int line, const char *fmt, ...);

#define LOG_INFO(fmt, ...) LogWrite(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) LogWrite(LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  LogWrite(LOG_LEVEL_ERR,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif
