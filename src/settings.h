#ifndef MYS3_SETTINGS_H
#define MYS3_SETTINGS_H

#include <gtk/gtk.h>

#include "logging.h"

typedef struct {
  gchar *endpoint;
  gchar *region;
  gchar *bucket;
  gboolean use_ssl;
  gboolean use_path_style;
  gboolean logging_enabled;
  LogLevel log_level;
} MyS3Settings;

MyS3Settings *settings_load(void);
void settings_save(MyS3Settings *settings);
void settings_free(MyS3Settings *settings);

#endif // MYS3_SETTINGS_H
