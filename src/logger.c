/* logger.c — T60: Dosya + stdout log sistemi */

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define LOG_FILE "game.log"

static FILE *g_logFile = NULL;

static const char *level_tag(LogLevel lvl) {
    switch (lvl) {
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_ERR:  return "ERR ";
        default:             return "    ";
    }
}

void InitLogger(void) {
    g_logFile = fopen(LOG_FILE, "a");
}

void CloseLogger(void) {
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}

void LogWrite(LogLevel level, const char *file, int line, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[16];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", t);

    /* Dosya adından yol kısmını çıkar */
    const char *base = file;
    for (const char *p = file; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;

    va_list args;

    /* stdout */
    printf("[%s] [%s] %s:%d  ", timebuf, level_tag(level), base, line);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");

    /* dosya */
    if (g_logFile) {
        fprintf(g_logFile, "[%s] [%s] %s:%d  ", timebuf, level_tag(level), base, line);
        va_start(args, fmt);
        vfprintf(g_logFile, fmt, args);
        va_end(args);
        fprintf(g_logFile, "\n");
        fflush(g_logFile);
    }
}
