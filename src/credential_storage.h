#ifndef MYS3_CREDENTIAL_STORAGE_H
#define MYS3_CREDENTIAL_STORAGE_H

#include <glib.h>

gboolean credential_storage_save(const gchar *endpoint,
                                 const gchar *access_key,
                                 const gchar *secret_key);

gboolean credential_storage_load(const gchar *endpoint,
                                 gchar **access_key,
                                 gchar **secret_key);

gboolean credential_storage_delete(const gchar *endpoint);

#endif // MYS3_CREDENTIAL_STORAGE_H
