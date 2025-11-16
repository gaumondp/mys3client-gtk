#ifndef LOGGING_H
#define LOGGING_H

#include <glib.h>

// Enum for log levels
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_DISABLED
} LogLevel;

void logging_init();
void logging_cleanup();
void logging_set_level(LogLevel level);
LogLevel logging_get_level();
const gchar* logging_get_directory();

#endif // LOGGING_H
