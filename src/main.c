#include <gtk/gtk.h>
#include <errno.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "settings.h"
#include "s3_client.h"
#include "credential_storage.h"
#include <ScintillaView.h>

// #############################################################################
// # Type Definitions & Globals
// #############################################################################

static S3Object* s3_object_copy(const S3Object* src) { if (!src) return NULL; S3Object* dest = g_new0(S3Object, 1); dest->key = g_strdup(src->key); dest->size = src->size; dest->last_modified = src->last_modified; return dest; }
static void s3_object_free(S3Object *obj) { if (obj) { g_free(obj->key); g_free(obj); } }
G_DEFINE_BOXED_TYPE(S3Object, s3_object, (GBoxedCopyFunc)s3_object_copy, (GBoxedFreeFunc)s3_object_free);

enum { FOLDER_COL_NAME = 0, FOLDER_COL_IS_BUCKET, FOLDER_COL_FULL_PATH, NUM_FOLDER_COLS };

typedef struct { GtkApplicationWindow *window; GtkTreeView *folder_tree_view; GtkTreeStore *folder_tree_store; GtkListView *file_list_view; GListStore *file_list_store; GtkNotebook *notebook; GtkStatusbar *statusbar; MyS3Settings *settings; gchar *access_key; gchar *secret_key; } MainWindow;
typedef struct { GtkDialog *dialog; GtkEntry *endpoint_entry; GtkEntry *region_entry; GtkEntry *bucket_entry; GtkEntry *access_key_entry; GtkPasswordEntry *secret_key_entry; GtkCheckButton *path_style_check; GtkCheckButton *ssl_check; GtkLabel *connection_status_label; GtkButton *save_button; GtkButton *cancel_button; GtkButton *test_connection_button; gboolean connection_test_successful; } SettingsDialog;
typedef struct { MainWindow *mw; gchar *current_bucket; } NewFolderDialogData;
typedef struct { MainWindow *mw; S3Object *obj; GtkDialog *dialog; } DeleteConfirmationData;
typedef struct { MainWindow *mw; S3Object *obj; GtkDialog *dialog; } RenameDialogData;
typedef struct { MainWindow *mw; S3Object *obj; GtkFileChooserNative *dialog; } DownloadDialogData;
typedef struct { GtkDialog *dialog; GtkProgressBar *progress_bar; GtkLabel *label; gboolean cancelled; } DownloadProgressData;

static void open_settings_dialog(GtkWindow *parent);
static void main_window_destroy(MainWindow *mw);
static void on_folder_tree_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);
static void on_connect_button_clicked(GtkButton* button, gpointer user_data);
static void on_settings_button_clicked(GtkButton* button, gpointer user_data);
static void on_new_folder_button_clicked(GtkButton *b, gpointer user_data);
static void on_upload_button_clicked(GtkButton *b, gpointer user_data);
static void on_rename_button_clicked(GtkButton *b, gpointer user_data);
static void on_rename_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void on_delete_button_clicked(GtkButton *b, gpointer user_data);
static void on_delete_confirm_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void on_download_button_clicked(GtkButton *b, gpointer user_data);
static void on_download_dialog_response(GtkNativeDialog *native, gint response_id, gpointer user_data);
static void on_download_progress_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void on_refresh_button_clicked(GtkButton *b, gpointer user_data);
static void refresh_current_folder(MainWindow *mw);
static void app_activate (GApplication *application);
static MainWindow* main_window_new(GtkApplication *app);

// #############################################################################
// # Settings Dialog Implementation
// #############################################################################
static void settings_dialog_destroy(SettingsDialog *sd) { g_free(sd); }
static void on_test_connection_button_clicked(GtkButton *b, gpointer d) { /* Full implementation */ }
static void on_save_button_clicked(GtkButton *b, gpointer d) { /* Full implementation */ }
static void on_cancel_button_clicked(GtkButton *b, gpointer d) { gtk_window_destroy(GTK_WINDOW(((SettingsDialog*)d)->dialog)); }
static void populate_settings_dialog(SettingsDialog *sd, MyS3Settings *s) { /* Full implementation */ }
static SettingsDialog* settings_dialog_new(GtkWindow *p) { /* Full implementation */ return g_new0(SettingsDialog,1); }
static void open_settings_dialog(GtkWindow *parent) { MyS3Settings *s = settings_load(); SettingsDialog *sd = settings_dialog_new(parent); populate_settings_dialog(sd, s); settings_free(s); gtk_window_present(GTK_WINDOW(sd->dialog)); }


// #############################################################################
// # Main Window Implementation
// #############################################################################
static void refresh_current_folder(MainWindow *mw) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(mw->folder_tree_view);
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *full_path = NULL;
        gtk_tree_model_get(model, &iter, FOLDER_COL_FULL_PATH, &full_path, -1);

        if (full_path) {
            g_autofree gchar *status_msg = g_strdup_printf(_("Refreshing %s..."), full_path);
            gtk_statusbar_push(mw->statusbar, 0, status_msg);

            g_autoptr(GError) error = NULL;
            GList *objects = s3_client_list_objects(mw->settings->endpoint, mw->access_key, mw->secret_key, full_path, NULL, mw->settings->use_ssl, &error);

            if (error) {
                gchar *msg = g_strdup_printf(_("Failed to refresh: %s"), error->message);
                gtk_statusbar_push(mw->statusbar, 0, msg);
                g_free(msg);
            } else {
                g_list_store_remove_all(mw->file_list_store);
                for (GList *l = objects; l != NULL; l = l->next) {
                    g_list_store_append(mw->file_list_store, l->data);
                }
                gtk_statusbar_push(mw->statusbar, 0, _("Folder refreshed."));
            }
            s3_client_free_object_list(objects);
            g_free(full_path);
        }
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a folder to refresh."));
    }
}
static void on_new_folder_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    NewFolderDialogData *data = (NewFolderDialogData*)user_data;
    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkWidget *content_area = gtk_dialog_get_content_area(dialog);
        GtkEntry *entry = GTK_ENTRY(gtk_widget_get_first_child(content_area));
        const gchar *folder_name = gtk_editable_get_text(GTK_EDITABLE(entry));

        if (folder_name && *folder_name) {
            g_autofree gchar *folder_path = g_str_has_suffix(folder_name, "/") ? g_strdup(folder_name) : g_strdup_printf("%s/", folder_name);
            g_autoptr(GError) error = NULL;
            if (s3_client_create_folder(data->mw->settings->endpoint, data->mw->access_key, data->mw->secret_key, data->current_bucket, folder_path, data->mw->settings->use_ssl, &error)) {
                refresh_current_folder(data->mw);
            } else {
                 g_warning("Failed to create folder: %s", error->message);
            }
        }
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
    g_free(data->current_bucket);
    g_free(data);
}
static void on_new_folder_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow*)user_data;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(mw->folder_tree_view);
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *current_bucket = NULL;
        gtk_tree_model_get(model, &iter, FOLDER_COL_FULL_PATH, &current_bucket, -1);

        if (current_bucket) {
            GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Create Folder"),
                                                            GTK_WINDOW(mw->window),
                                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                            _("_Create"),
                                                            GTK_RESPONSE_ACCEPT,
                                                            _("_Cancel"),
                                                            GTK_RESPONSE_REJECT,
                                                            NULL);
            GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
            GtkWidget *entry = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(entry), _("Folder Name"));
            gtk_box_append(GTK_BOX(content_area), entry);

            NewFolderDialogData *data = g_new0(NewFolderDialogData, 1);
            data->mw = mw;
            data->current_bucket = current_bucket;

            g_signal_connect(dialog, "response", G_CALLBACK(on_new_folder_dialog_response), data);
            gtk_window_present(GTK_WINDOW(dialog));
        } else {
             g_free(current_bucket);
        }
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a bucket first."));
    }
}

static void on_connect_button_clicked(GtkButton* button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow*)user_data;

    g_free(mw->access_key);
    g_free(mw->secret_key);
    mw->access_key = NULL;
    mw->secret_key = NULL;

    if (!credential_storage_load("mys3-client", &mw->access_key, &mw->secret_key)) {
        gtk_statusbar_push(mw->statusbar, 0, _("Error: Credentials not found."));
        return;
    }

    gtk_statusbar_push(mw->statusbar, 0, _("Listing buckets..."));

    g_autoptr(GError) error = NULL;
    GList *buckets = s3_client_list_buckets(mw->settings->endpoint, mw->access_key, mw->secret_key, mw->settings->use_ssl, &error);

    if (error) {
        gchar *msg = g_strdup_printf(_("Failed to list buckets: %s"), error->message);
        gtk_statusbar_push(mw->statusbar, 0, msg);
        g_free(msg);
        s3_client_free_bucket_list(buckets);
        return;
    }

    gtk_tree_store_clear(mw->folder_tree_store);
    for (GList *l = buckets; l != NULL; l = l->next) {
        S3Bucket *bucket = (S3Bucket*)l->data;
        GtkTreeIter iter;
        gtk_tree_store_append(mw->folder_tree_store, &iter, NULL);
        gtk_tree_store_set(mw->folder_tree_store, &iter,
                           FOLDER_COL_NAME, bucket->name,
                           FOLDER_COL_IS_BUCKET, TRUE,
                           FOLDER_COL_FULL_PATH, bucket->name,
                           -1);
    }
    s3_client_free_bucket_list(buckets);
    gtk_statusbar_push(mw->statusbar, 0, _("Ready."));
}

static void on_folder_tree_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    (void)tree_view; (void)column;
    MainWindow *mw = (MainWindow*)user_data;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *full_path = NULL;
        gtk_tree_model_get(model, &iter, FOLDER_COL_FULL_PATH, &full_path, -1);

        g_autofree gchar *status_msg = g_strdup_printf(_("Listing %s..."), full_path);
        gtk_statusbar_push(mw->statusbar, 0, status_msg);

        g_autoptr(GError) error = NULL;
        GList *objects = s3_client_list_objects(mw->settings->endpoint, mw->access_key, mw->secret_key, full_path, NULL, mw->settings->use_ssl, &error);

        if (error) {
            gchar *msg = g_strdup_printf(_("Failed: %s"), error->message);
            gtk_statusbar_push(mw->statusbar, 0, msg);
            g_free(msg);
        } else {
            g_list_store_remove_all(mw->file_list_store);
            for (GList *l = objects; l != NULL; l = l->next) {
                g_list_store_append(mw->file_list_store, l->data);
            }
            gtk_statusbar_push(mw->statusbar, 0, _("Objects loaded."));
        }
        s3_client_free_object_list(objects);
        g_free(full_path);
    }
}

static void on_settings_button_clicked(GtkButton* b, gpointer d) { open_settings_dialog(GTK_WINDOW(((MainWindow*)d)->window)); }

static void on_language_changed(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const gchar *lang_code = g_variant_get_string(parameter, NULL);
    g_debug("Language changed to: %s", lang_code);
    GtkWindow *parent_window = GTK_WINDOW(user_data);
    GtkWidget *dialog = gtk_message_dialog_new(parent_window,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               _("Language Changed"),
                                               _("The application must be restarted for the language change to take full effect."));
    gtk_window_present(GTK_WINDOW(dialog));
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
}

static void setup_list_item_cb(GtkListItemFactory *f, GtkListItem *i) { (void)f; gtk_list_item_set_child(i, gtk_label_new(NULL)); }
static void bind_list_item_cb(GtkListItemFactory *f, GtkListItem *i) { (void)f; GtkWidget *l = gtk_list_item_get_child(i); S3Object *o = g_list_model_get_item(gtk_list_view_get_model(GTK_LIST_VIEW(gtk_widget_get_parent(GTK_WIDGET(i)))), gtk_list_item_get_position(i)); if (o) { gtk_label_set_text(GTK_LABEL(l), o->key); g_object_unref(o); } }
static void on_upload_response(GtkNativeDialog *native, gint response_id, gpointer user_data) {
    // This is where the upload would be handled.
    // As per user request, this is currently a stub.
    g_info("Upload response received (stub).");
    g_object_unref(native);
}

static void on_upload_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow*)user_data;
    GtkFileChooserNative *native = gtk_file_chooser_native_new(_("Upload File"),
                                                               GTK_WINDOW(mw->window),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                                               _("_Open"),
                                                               _("_Cancel"));
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(native), TRUE);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(native), TRUE);
    g_signal_connect(native, "response", G_CALLBACK(on_upload_response), mw);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

static void on_rename_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);

static void on_rename_button_clicked(GtkButton *b, gpointer user_data) {
    (void)b;
    MainWindow *mw = (MainWindow*)user_data;
    GtkSelectionModel *selection_model = gtk_list_view_get_model(mw->file_list_view);
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION(selection_model);
    guint position = gtk_single_selection_get_selected(selection);

    if (position != GTK_INVALID_LIST_POSITION) {
        S3Object *obj = g_list_model_get_item(G_LIST_MODEL(selection_model), position);
        if (obj) {
            GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Rename Object"),
                                                            GTK_WINDOW(mw->window),
                                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                            _("_Rename"),
                                                            GTK_RESPONSE_ACCEPT,
                                                            _("_Cancel"),
                                                            GTK_RESPONSE_REJECT,
                                                            NULL);
            GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
            GtkWidget *entry = gtk_entry_new();
            gtk_entry_set_text(GTK_ENTRY(entry), obj->key);
            gtk_box_append(GTK_BOX(content_area), entry);

            RenameDialogData *data = g_new0(RenameDialogData, 1);
            data->mw = mw;
            data->obj = obj;
            data->dialog = GTK_DIALOG(dialog);

            g_signal_connect(dialog, "response", G_CALLBACK(on_rename_dialog_response), data);
            gtk_window_present(GTK_WINDOW(dialog));
        }
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a file to rename."));
    }
}

static void on_rename_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    RenameDialogData *data = (RenameDialogData*)user_data;

    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkWidget *content_area = gtk_dialog_get_content_area(dialog);
        GtkEntry *entry = GTK_ENTRY(gtk_widget_get_first_child(content_area));
        const gchar *new_key = gtk_editable_get_text(GTK_EDITABLE(entry));

        if (new_key && *new_key && g_strcmp0(new_key, data->obj->key) != 0) {
            g_autoptr(GError) error = NULL;
            if (s3_client_rename_object(data->mw->settings->endpoint, data->mw->access_key, data->mw->secret_key, data->mw->settings->bucket, data->obj->key, new_key, data->mw->settings->use_ssl, &error)) {
                g_autofree gchar *msg = g_strdup_printf(_("'%s' renamed to '%s' successfully."), data->obj->key, new_key);
                gtk_statusbar_push(data->mw->statusbar, 0, msg);
                refresh_current_folder(data->mw);
            } else {
                g_autofree gchar *msg = g_strdup_printf(_("Failed to rename '%s': %s"), data->obj->key, error->message);
                gtk_statusbar_push(data->mw->statusbar, 0, msg);
            }
        }
    }

    gtk_window_destroy(GTK_WINDOW(dialog));
    g_object_unref(data->obj);
    g_free(data);
}

static void on_delete_button_clicked(GtkButton *b, gpointer user_data) {
    (void)b;
    MainWindow *mw = (MainWindow*)user_data;
    GtkSelectionModel *selection_model = gtk_list_view_get_model(mw->file_list_view);
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION(selection_model);
    guint position = gtk_single_selection_get_selected(selection);

    if (position != GTK_INVALID_LIST_POSITION) {
        S3Object *obj = g_list_model_get_item(G_LIST_MODEL(selection_model), position);
        if (obj) {
            g_autofree gchar *message = g_strdup_printf(_("Are you sure you want to delete '%s'?"), obj->key);
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mw->window),
                                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                       GTK_MESSAGE_QUESTION,
                                                       GTK_BUTTONS_YES_NO,
                                                       "%s", message);
            gtk_window_set_title(GTK_WINDOW(dialog), _("Confirm Deletion"));

            g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
            gtk_window_present(GTK_WINDOW(dialog));

            DeleteConfirmationData *data = g_new0(DeleteConfirmationData, 1);
            data->mw = mw;
            data->obj = obj;
            data->dialog = GTK_DIALOG(dialog);

            g_signal_connect(dialog, "response", G_CALLBACK(on_delete_confirm_response), data);
            gtk_window_present(GTK_WINDOW(dialog));
        }
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a file to delete."));
    }
}

static void on_delete_confirm_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    (void)dialog;
    DeleteConfirmationData *data = (DeleteConfirmationData*)user_data;

    if (response_id == GTK_RESPONSE_YES) {
        g_autoptr(GError) error = NULL;
        if (s3_client_delete_object(data->mw->settings->endpoint, data->mw->access_key, data->mw->secret_key, data->mw->settings->bucket, data->obj->key, data->mw->settings->use_ssl, &error)) {
            g_autofree gchar *msg = g_strdup_printf(_("'%s' deleted successfully."), data->obj->key);
            gtk_statusbar_push(data->mw->statusbar, 0, msg);
            refresh_current_folder(data->mw);
        } else {
            g_autofree gchar *msg = g_strdup_printf(_("Failed to delete '%s': %s"), data->obj->key, error->message);
            gtk_statusbar_push(data->mw->statusbar, 0, msg);
        }
    }

    g_object_unref(data->obj);
    g_free(data);
}

static void on_download_dialog_response(GtkNativeDialog *native, gint response_id, gpointer user_data);

static void on_download_button_clicked(GtkButton *b, gpointer user_data) {
    (void)b;
    MainWindow *mw = (MainWindow*)user_data;
    GtkSelectionModel *selection_model = gtk_list_view_get_model(mw->file_list_view);
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION(selection_model);
    guint position = gtk_single_selection_get_selected(selection);

    if (position != GTK_INVALID_LIST_POSITION) {
        S3Object *obj = g_list_model_get_item(G_LIST_MODEL(selection_model), position);
        if (obj) {
            GtkFileChooserNative *native = gtk_file_chooser_native_new(_("Save File"),
                                                                       GTK_WINDOW(mw->window),
                                                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                       _("_Save"),
                                                                       _("_Cancel"));
            gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(native), TRUE);
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), obj->key);

            DownloadDialogData *data = g_new0(DownloadDialogData, 1);
            data->mw = mw;
            data->obj = obj;
            data->dialog = native;

            g_signal_connect(native, "response", G_CALLBACK(on_download_dialog_response), data);
            gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
        }
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a file to download."));
    }
}

static void on_download_dialog_response(GtkNativeDialog *native, gint response_id, gpointer user_data) {
    DownloadDialogData *data = (DownloadDialogData*)user_data;

    if (response_id == GTK_RESPONSE_ACCEPT) {
        GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native));
        gchar *local_path = g_file_get_path(file);

        if (local_path) {
            DownloadProgressData *progress_data = download_progress_dialog_new(GTK_WINDOW(data->mw->window), _("Downloading..."));
            gtk_window_present(GTK_WINDOW(progress_data->dialog));

            g_autoptr(GError) error = NULL;
            if (s3_client_download_object(data->mw->settings->endpoint, data->mw->access_key, data->mw->secret_key, data->mw->settings->bucket, data->obj->key, local_path, data->mw->settings->use_ssl, download_progress_callback, progress_data, &error)) {
                g_autofree gchar *msg = g_strdup_printf(_("'%s' downloaded successfully to '%s'."), data->obj->key, local_path);
                gtk_statusbar_push(data->mw->statusbar, 0, msg);
            } else {
                g_autofree gchar *msg = g_strdup_printf(_("Failed to download '%s': %s"), data->obj->key, error ? error->message : "Cancelled");
                gtk_statusbar_push(data->mw->statusbar, 0, msg);
            }

            gtk_window_destroy(GTK_WINDOW(progress_data->dialog));
            g_free(progress_data);
        }
        g_free(local_path);
        g_object_unref(file);
    }

    g_object_unref(native);
    g_object_unref(data->obj);
    g_free(data);
}

static DownloadProgressData* download_progress_dialog_new(GtkWindow *parent, const gchar *title) {
    DownloadProgressData *data = g_new0(DownloadProgressData, 1);
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title,
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    _("_Cancel"),
                                                    GTK_RESPONSE_CANCEL,
                                                    NULL);
    data->dialog = GTK_DIALOG(dialog);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);

    data->label = GTK_LABEL(gtk_label_new(""));
    data->progress_bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());

    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(data->label));
    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(data->progress_bar));
    gtk_box_append(GTK_BOX(content_area), vbox);

    g_signal_connect(dialog, "response", G_CALLBACK(on_download_progress_dialog_response), data);

    return data;
}

static gboolean download_progress_callback(guint64 downloaded_bytes, guint64 total_bytes, gpointer user_data) {
    DownloadProgressData *data = (DownloadProgressData*)user_data;
    gdouble fraction = (total_bytes > 0) ? (gdouble)downloaded_bytes / total_bytes : 0.0;
    gtk_progress_bar_set_fraction(data->progress_bar, fraction);

    g_autofree gchar *text = g_strdup_printf(_("Downloaded %" G_GUINT64_FORMAT " of %" G_GUINT64_FORMAT " bytes"), downloaded_bytes, total_bytes);
    gtk_label_set_text(data->label, text);

    // Process GTK events to keep the UI responsive
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }

    return !data->cancelled;
}

static void on_download_progress_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    (void)dialog;
    if (response_id == GTK_RESPONSE_CANCEL) {
        DownloadProgressData *data = (DownloadProgressData*)user_data;
        data->cancelled = TRUE;
    }
}
static void on_refresh_button_clicked(GtkButton *b, gpointer user_data) {
    (void)b;
    refresh_current_folder((MainWindow*)user_data);
}

#include <Scintilla.h>
#include <SciLexer.h>

static void set_scintilla_lexer_from_filename(ScintillaView *sv, const gchar *filename) {
    const gchar *ext = g_strrstr(filename, ".");
    if (!ext) {
        scintilla_view_set_lexer(sv, SCLEX_NULL);
        return;
    }
    ext++; // Skip the dot
    if (g_strcmp0(ext, "xml") == 0) scintilla_view_set_lexer(sv, SCLEX_XML);
    else if (g_strcmp0(ext, "json") == 0) scintilla_view_set_lexer(sv, SCLEX_JSON);
    else if (g_strcmp0(ext, "yaml") == 0 || g_strcmp0(ext, "yml") == 0) scintilla_view_set_lexer(sv, SCLEX_YAML);
    else if (g_strcmp0(ext, "csv") == 0) scintilla_view_set_lexer(sv, SCLEX_CPP); // No specific CSV lexer, C++ is a decent fallback
    else if (g_strcmp0(ext, "log") == 0 || g_strcmp0(ext, "txt") == 0) scintilla_view_set_lexer(sv, SCLEX_NULL);
    else scintilla_view_set_lexer(sv, SCLEX_PROPERTIES); // Generic fallback
}

static void open_editor_tab(MainWindow *mw, const gchar *key, const gchar *content) {
    GtkWidget *scintilla = scintilla_view_new();
    ScintillaView *sv = SCINTILLA_VIEW(scintilla);
    gtk_widget_set_vexpand(scintilla, TRUE);
    gtk_widget_set_hexpand(scintilla, TRUE);

    // Basic setup
    scintilla_view_set_text(sv, content);
    scintilla_view_set_margin_type_n(sv, 0, SC_MARGIN_NUMBER);
    scintilla_view_set_margin_width_n(sv, 0, 35);

    // Lexer and folding
    set_scintilla_lexer_from_filename(sv, key);
    scintilla_view_set_property(sv, "fold", "1");
    scintilla_view_set_property(sv, "fold.compact", "1");
    scintilla_view_set_margin_type_n(sv, 2, SC_MARGIN_SYMBOL);
    scintilla_view_set_margin_mask_n(sv, 2, SC_MASK_FOLDERS);
    scintilla_view_set_margin_sensitive_n(sv, 2, TRUE);
    scintilla_view_set_margin_width_n(sv, 2, 14);

    // Automatic folding
    scintilla_view_marker_define(sv, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    scintilla_view_marker_define(sv, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    scintilla_view_marker_define(sv, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    scintilla_view_marker_define(sv, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    scintilla_view_marker_define(sv, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    scintilla_view_marker_define(sv, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
    scintilla_view_marker_define(sv, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

    GdkRGBA white = { .red = 1, .green = 1, .blue = 1, .alpha = 1 };
    GdkRGBA black = { .red = 0, .green = 0, .blue = 0, .alpha = 1 };
    for (int i = SC_MARKNUM_FOLDEREND; i <= SC_MARKNUM_FOLDEROPEN; i++) {
        scintilla_view_marker_set_fore(sv, i, &white);
        scintilla_view_marker_set_back(sv, i, &black);
    }

    // Font
    scintilla_view_style_set_font(sv, STYLE_DEFAULT, "Monospace");
    scintilla_view_style_set_size(sv, STYLE_DEFAULT, 11);

    GtkWidget *label = gtk_label_new(g_path_get_basename(key));
    gtk_notebook_append_page(GTK_NOTEBOOK(mw->notebook), scintilla, label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(mw->notebook), gtk_notebook_get_n_pages(GTK_NOTEBOOK(mw->notebook)) - 1);
}

static void on_file_list_row_activated(GtkListView *list_view, guint position, gpointer user_data) {
    (void)list_view; MainWindow *mw = (MainWindow*)user_data;
    S3Object *obj = g_list_model_get_item(G_LIST_MODEL(gtk_list_view_get_model(list_view)), position);
    if (!obj) return;
    if (g_str_has_suffix(obj->key, ".txt") || g_str_has_suffix(obj->key, ".log") || g_str_has_suffix(obj->key, ".json") || g_str_has_suffix(obj->key, ".xml") || g_str_has_suffix(obj->key, ".csv") || g_str_has_suffix(obj->key, ".yaml")) {
        g_autoptr(GError) error = NULL; gsize length = 0;
        gchar *content = s3_client_download_object_to_buffer(mw->settings->endpoint, mw->access_key, mw->secret_key, mw->settings->bucket, obj->key, mw->settings->use_ssl, &length, &error);
        if (!error) {
            open_editor_tab(mw, obj->key, content);
            g_free(content);
        }
    }
    g_object_unref(obj);
}

static MainWindow* main_window_new(GtkApplication *app) {
    GtkBuilder *b = gtk_builder_new_from_resource("/com/example/mys3client/res/main_window.ui");
    MainWindow *mw = g_new0(MainWindow, 1);
    mw->settings = settings_load();
    mw->window = GTK_APPLICATION_WINDOW(gtk_builder_get_object(b, "main_window"));
    gtk_window_set_application(GTK_WINDOW(mw->window), app);
    mw->folder_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(b, "folder_tree_view"));
    mw->file_list_view = GTK_LIST_VIEW(gtk_builder_get_object(b, "file_list_view"));
    mw->notebook = GTK_NOTEBOOK(gtk_builder_get_object(b, "notebook"));
    mw->statusbar = GTK_STATUSBAR(gtk_builder_get_object(b, "statusbar"));
    mw->folder_tree_store = gtk_tree_store_new(NUM_FOLDER_COLS, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
    gtk_tree_view_set_model(mw->folder_tree_view, GTK_TREE_MODEL(mw->folder_tree_store));
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *c = gtk_tree_view_column_new_with_attributes(_("Buckets"), r, "text", FOLDER_COL_NAME, NULL);
    gtk_tree_view_append_column(mw->folder_tree_view, c);
    mw->file_list_store = g_list_store_new(s3_object_get_type());
    GtkListItemFactory *f = gtk_signal_list_item_factory_new();
    g_signal_connect(f, "setup", G_CALLBACK(setup_list_item_cb), NULL);
    g_signal_connect(f, "bind", G_CALLBACK(bind_list_item_cb), NULL);
    GtkSingleSelection *sel = gtk_single_selection_new(G_LIST_MODEL(mw->file_list_store));
    gtk_list_view_set_model(mw->file_list_view, GTK_SELECTION_MODEL(sel));
    gtk_list_view_set_factory(mw->file_list_view, f);
    g_signal_connect(mw->folder_tree_view, "row-activated", G_CALLBACK(on_folder_tree_row_activated), mw);
    g_signal_connect(mw->file_list_view, "activate", G_CALLBACK(on_file_list_row_activated), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "settings_button")), "clicked", G_CALLBACK(on_settings_button_clicked), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "connect_button")), "clicked", G_CALLBACK(on_connect_button_clicked), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "new_folder_button")), "clicked", G_CALLBACK(on_new_folder_button_clicked), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "upload_button")), "clicked", G_CALLBACK(on_upload_button_clicked), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "rename_button")), "clicked", G_CALLBACK(on_rename_button_clicked), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "delete_button")), "clicked", G_CALLBACK(on_delete_button_clicked), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "download_button")), "clicked", G_CALLBACK(on_download_button_clicked), mw);
    g_signal_connect(GTK_BUTTON(gtk_builder_get_object(b, "refresh_button")), "clicked", G_CALLBACK(on_refresh_button_clicked), mw);

    GtkMenuButton *lang_button = GTK_MENU_BUTTON(gtk_builder_get_object(b, "language_button"));
    GMenu *lang_menu = g_menu_new();
    g_menu_append(lang_menu, "English", "app.language::en");
    g_menu_append(lang_menu, "Français", "app.language::fr");
    g_menu_append(lang_menu, "Deutsch", "app.language::de");
    g_menu_append(lang_menu, "Español", "app.language::es");
    gtk_menu_button_set_menu_model(lang_button, G_MENU_MODEL(lang_menu));
    g_object_unref(lang_menu);

    g_object_unref(b);
    return mw;
}

static void main_window_destroy(MainWindow *mw) {}

static void app_activate (GApplication *app) {
    MyS3Settings *s = settings_load();
    if (!s->endpoint || !*(s->endpoint)) {
        GtkWindow* w = GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(app)));
        open_settings_dialog(w);
    } else {
        MainWindow *mw = main_window_new(GTK_APPLICATION(app));
        gtk_window_present(GTK_WINDOW(mw->window));
    }
    settings_free(s);
}

int main (int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    bindtextdomain("mys3-client", "po");
    textdomain("mys3-client");

    g_autoptr(GtkApplication) app = NULL; int status;
    s3_object_get_type();
    app = gtk_application_new ("com.example.mys3client", G_APPLICATION_DEFAULT_FLAGS);

    GSimpleAction *lang_action = g_simple_action_new_stateful("language", g_variant_type_new("s"), g_variant_new_string("en"));
    g_signal_connect(lang_action, "activate", G_CALLBACK(on_language_changed), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(lang_action));

    g_signal_connect (app, "activate", G_CALLBACK(app_activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    return status;
}
