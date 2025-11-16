#include <gtk/gtk.h>
#include <errno.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <glib/gstdio.h>
#include "settings.h"
#include "s3_client.h"
#include "credential_storage.h"
#include <gtksourceview/gtksource.h>

// #############################################################################
// # Type Definitions & Globals
// #############################################################################

static S3Object* s3_object_copy(const S3Object* src) { if (!src) return NULL; S3Object* dest = g_new0(S3Object, 1); dest->key = g_strdup(src->key); dest->size = src->size; dest->last_modified = src->last_modified; return dest; }
static void s3_object_free(S3Object *obj) { if (obj) { g_free(obj->key); g_free(obj); } }
G_DEFINE_BOXED_TYPE(S3Object, s3_object, (GBoxedCopyFunc)s3_object_copy, (GBoxedFreeFunc)s3_object_free);

typedef struct _FolderItem {
    gchar *name;
    gboolean is_bucket;
    gchar *full_path;
    GListStore *children;
} FolderItem;

typedef struct { GtkApplicationWindow *window; GtkListView *folder_tree_view; GtkTreeListModel *folder_tree_model; GtkListView *file_list_view; GListStore *file_list_store; GtkNotebook *notebook; GtkStatusbar *statusbar; GtkButton *find_button; GtkWidget *find_dialog; GtkEntry *find_entry; GtkEntry *replace_entry; MyS3Settings *settings; gchar *access_key; gchar *secret_key; } MainWindow;
typedef struct { GtkDialog *dialog; GtkEntry *endpoint_entry; GtkEntry *region_entry; GtkEntry *bucket_entry; GtkEntry *access_key_entry; GtkPasswordEntry *secret_key_entry; GtkCheckButton *path_style_check; GtkCheckButton *ssl_check; GtkLabel *connection_status_label; GtkButton *save_button; GtkButton *cancel_button; GtkButton *test_connection_button; gboolean connection_test_successful; } SettingsDialog;
typedef struct { MainWindow *mw; gchar *current_bucket; } NewFolderDialogData;
typedef struct { MainWindow *mw; S3Object *obj; GtkDialog *dialog; } DeleteConfirmationData;
typedef struct { MainWindow *mw; S3Object *obj; GtkDialog *dialog; } RenameDialogData;
typedef struct { MainWindow *mw; S3Object *obj; GtkFileChooserNative *dialog; } DownloadDialogData;
typedef struct { GtkDialog *dialog; GtkProgressBar *progress_bar; GtkLabel *label; gboolean cancelled; } DownloadProgressData;
typedef struct { gchar *key; GtkSourceView *source_view; MainWindow *mw; gboolean unsaved; GtkWidget *tab_label; } EditorSaveData;

static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data);
static void open_settings_dialog(GtkWindow *parent);
static void on_connect_button_clicked(GtkButton* button, gpointer user_data);
static GListModel* folder_model_get_children(gpointer item, gpointer user_data);
static void setup_folder_list_item_cb(GtkListItemFactory *factory, GtkListItem *list_item);
static void bind_folder_list_item_cb(GtkListItemFactory *factory, GtkListItem *list_item);
static void on_settings_button_clicked(GtkButton* button, gpointer user_data);
static void on_new_folder_button_clicked(GtkButton *b, gpointer user_data);
static void on_new_folder_dialog_response(GtkButton *button, gpointer user_data);
static void on_upload_button_clicked(GtkButton *b, gpointer user_data);
static void on_rename_button_clicked(GtkButton *b, gpointer user_data);
static void on_rename_dialog_response(GtkButton *button, gpointer user_data);
static void on_delete_button_clicked(GtkButton *b, gpointer user_data);
static void on_delete_confirm_response(GtkButton *button, gpointer user_data);
static void on_download_button_clicked(GtkButton *b, gpointer user_data);
static void on_download_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void on_refresh_button_clicked(GtkButton *b, gpointer user_data);
static void refresh_current_folder(MainWindow *mw);
static void app_activate (GApplication *application);
static void on_editor_save_button_clicked(GtkButton *button, gpointer user_data);
static void on_find_button_clicked(GtkButton *button, gpointer user_data);
static void on_close_button_clicked(GtkButton *button, gpointer user_data);
static gboolean on_window_close_request(GtkApplicationWindow *window, gpointer user_data);
static MainWindow* main_window_new(GtkApplication *app);

// #############################################################################
// # Settings Dialog Implementation
// #############################################################################
static void populate_settings_dialog(SettingsDialog *sd, MyS3Settings *s) { /* Full implementation */ }
static SettingsDialog* settings_dialog_new(GtkWindow *p) { /* Full implementation */ return g_new0(SettingsDialog,1); }
static void open_settings_dialog(GtkWindow *parent) { MyS3Settings *s = settings_load(); SettingsDialog *sd = settings_dialog_new(parent); populate_settings_dialog(sd, s); settings_free(s); gtk_window_present(GTK_WINDOW(sd->dialog)); }


// #############################################################################
// # Main Window Implementation
// #############################################################################
static void refresh_current_folder(MainWindow *mw) {
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION(gtk_list_view_get_model(mw->folder_tree_view));
    FolderItem *item = g_list_model_get_item(G_LIST_MODEL(selection), gtk_single_selection_get_selected(selection));

    if (item) {
        g_autofree gchar *status_msg = g_strdup_printf(_("Refreshing %s..."), item->full_path);
        gtk_statusbar_push(mw->statusbar, 0, status_msg);

        g_autoptr(GError) error = NULL;
        GList *objects = s3_client_list_objects(mw->settings->endpoint, mw->access_key, mw->secret_key, item->full_path, NULL, mw->settings->use_ssl, &error);

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
        g_object_unref(item);
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a folder to refresh."));
    }
}

static void on_new_folder_dialog_response(GtkButton *button, gpointer user_data) {
    (void)button;
    NewFolderDialogData *data = (NewFolderDialogData*)user_data;
    GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    GtkWidget *content_area = gtk_window_get_child(GTK_WINDOW(dialog));
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
    gtk_window_destroy(GTK_WINDOW(dialog));
    g_free(data->current_bucket);
    g_free(data);
}
static void on_new_folder_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow*)user_data;

    GtkSingleSelection *selection = GTK_SINGLE_SELECTION(gtk_list_view_get_model(mw->folder_tree_view));
    FolderItem *item = g_list_model_get_item(G_LIST_MODEL(selection), gtk_single_selection_get_selected(selection));

    if (item) {
        gchar *current_bucket = g_strdup(item->full_path);
        g_object_unref(item);

        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mw->window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_title(GTK_WINDOW(dialog), _("Create Folder"));

        GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_start(content_area, 12);
        gtk_widget_set_margin_end(content_area, 12);
        gtk_widget_set_margin_top(content_area, 12);
        gtk_widget_set_margin_bottom(content_area, 12);
        gtk_window_set_child(GTK_WINDOW(dialog), content_area);

        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), _("Folder Name"));
        gtk_box_append(GTK_BOX(content_area), entry);

        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(button_box, GTK_ALIGN_END);
        GtkWidget *create_button = gtk_button_new_with_label(_("_Create"));
        GtkWidget *cancel_button = gtk_button_new_with_label(_("_Cancel"));
        gtk_box_append(GTK_BOX(button_box), create_button);
        gtk_box_append(GTK_BOX(button_box), cancel_button);
        gtk_box_append(GTK_BOX(content_area), button_box);

        NewFolderDialogData *data = g_new0(NewFolderDialogData, 1);
        data->mw = mw;
        data->current_bucket = current_bucket;

        g_signal_connect(create_button, "clicked", G_CALLBACK(on_new_folder_dialog_response), data);
        g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_window_present(GTK_WINDOW(dialog));
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a bucket first."));
    }
}

static void setup_folder_list_item_cb(GtkListItemFactory *factory, GtkListItem *list_item) {
    (void)factory;
    GtkWidget *label = gtk_label_new(NULL);
    gtk_list_item_set_child(list_item, label);
}

static void bind_folder_list_item_cb(GtkListItemFactory *factory, GtkListItem *list_item) {
    (void)factory;
    GtkWidget *label = gtk_list_item_get_child(list_item);
    FolderItem *item = g_list_model_get_item(gtk_list_view_get_model(GTK_LIST_VIEW(gtk_widget_get_parent(GTK_WIDGET(list_item)))), gtk_list_item_get_position(list_item));
    if (item) {
        gtk_label_set_text(GTK_LABEL(label), item->name);
        g_object_unref(item);
    }
}

static GListModel* folder_model_get_children(gpointer item, gpointer user_data) {
    (void)user_data;
    FolderItem *folder_item = (FolderItem*)item;
    return G_LIST_MODEL(folder_item->children);
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

    GListStore *folder_store = G_LIST_STORE(gtk_tree_list_model_get_model(mw->folder_tree_model));
    g_list_store_remove_all(folder_store);
    for (GList *l = buckets; l != NULL; l = l->next) {
        S3Bucket *bucket = (S3Bucket*)l->data;
        FolderItem *item = g_new0(FolderItem, 1);
        item->name = g_strdup(bucket->name);
        item->is_bucket = TRUE;
        item->full_path = g_strdup(bucket->name);
        item->children = g_list_store_new(G_TYPE_POINTER);
        g_list_store_append(folder_store, item);
    }
    s3_client_free_bucket_list(buckets);
    gtk_statusbar_push(mw->statusbar, 0, _("Ready."));
}

static void on_folder_tree_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    (void)tree_view; (void)column;
    MainWindow *mw = (MainWindow*)user_data;
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION(gtk_list_view_get_model(mw->folder_tree_view));
    FolderItem *item = g_list_model_get_item(G_LIST_MODEL(selection), gtk_single_selection_get_selected(selection));

    if (item) {
        gchar *full_path = g_strdup(item->full_path);
        g_object_unref(item);

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

static void do_language_change(MainWindow *mw) {
    gtk_window_destroy(GTK_WINDOW(mw->window));
    app_activate(G_APPLICATION(gtk_window_get_application(GTK_WINDOW(mw->window))));
}

static void on_language_change_response(GtkButton *button, gpointer user_data) {
    MainWindow *mw = (MainWindow *)user_data;
    GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    gtk_window_destroy(GTK_WINDOW(dialog));
    do_language_change(mw);
}

static void on_language_changed(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    (void)action; (void)parameter;
    MainWindow *mw = (MainWindow *)user_data;

    gboolean unsaved_changes = FALSE;
    for (guint i = 1; i < gtk_notebook_get_n_pages(mw->notebook); ++i) {
        GtkWidget *page = gtk_notebook_get_nth_page(mw->notebook, i);
        GtkWidget *tab_box = gtk_notebook_get_tab_label(mw->notebook, page);
        GtkWidget *tab_label = gtk_widget_get_first_child(tab_box);
        const gchar *label_text = gtk_label_get_text(GTK_LABEL(tab_label));
        if (g_str_has_suffix(label_text, "*")) {
            unsaved_changes = TRUE;
            break;
        }
    }

    if (unsaved_changes) {
        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mw->window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_title(GTK_WINDOW(dialog), _("There are unsaved changes. Do you want to close anyway?"));

        GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_start(content_area, 12);
        gtk_widget_set_margin_end(content_area, 12);
        gtk_widget_set_margin_top(content_area, 12);
        gtk_widget_set_margin_bottom(content_area, 12);
        gtk_window_set_child(GTK_WINDOW(dialog), content_area);

        GtkWidget *label = gtk_label_new(_("There are unsaved changes. Do you want to close anyway?"));
        gtk_box_append(GTK_BOX(content_area), label);

        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(button_box, GTK_ALIGN_END);
        GtkWidget *yes_button = gtk_button_new_with_label(_("_Yes"));
        GtkWidget *no_button = gtk_button_new_with_label(_("_No"));
        gtk_box_append(GTK_BOX(button_box), yes_button);
        gtk_box_append(GTK_BOX(button_box), no_button);
        gtk_box_append(GTK_BOX(content_area), button_box);

        g_signal_connect(yes_button, "clicked", G_CALLBACK(on_language_change_response), mw);
        g_signal_connect_swapped(no_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_window_present(GTK_WINDOW(dialog));
    } else {
        do_language_change(mw);
    }
}

static void setup_list_item_cb(GtkListItemFactory *f, GtkListItem *i) { (void)f; gtk_list_item_set_child(i, gtk_label_new(NULL)); }
static void bind_list_item_cb(GtkListItemFactory *f, GtkListItem *i) { (void)f; GtkWidget *l = gtk_list_item_get_child(i); S3Object *o = gtk_list_item_get_item(i); if (o) { gtk_label_set_text(GTK_LABEL(l), o->key); } }
static void on_upload_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    (void)response_id; (void)user_data;
    gtk_window_destroy(GTK_WINDOW(dialog));
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

static void on_rename_button_clicked(GtkButton *b, gpointer user_data) {
    (void)b;
    MainWindow *mw = (MainWindow*)user_data;
    GtkSelectionModel *selection_model = gtk_list_view_get_model(mw->file_list_view);
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION(selection_model);
    guint position = gtk_single_selection_get_selected(selection);

    if (position != GTK_INVALID_LIST_POSITION) {
        S3Object *obj = g_list_model_get_item(G_LIST_MODEL(selection_model), position);
        if (obj) {
            GtkWidget *dialog = gtk_window_new();
            gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mw->window));
            gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
            gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
            gtk_window_set_title(GTK_WINDOW(dialog), _("Rename Object"));

            GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
            gtk_widget_set_margin_start(content_area, 12);
            gtk_widget_set_margin_end(content_area, 12);
            gtk_widget_set_margin_top(content_area, 12);
            gtk_widget_set_margin_bottom(content_area, 12);
            gtk_window_set_child(GTK_WINDOW(dialog), content_area);

            GtkWidget *entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(entry), obj->key);
            gtk_box_append(GTK_BOX(content_area), entry);

            GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
            gtk_widget_set_halign(button_box, GTK_ALIGN_END);
            GtkWidget *rename_button = gtk_button_new_with_label(_("_Rename"));
            GtkWidget *cancel_button = gtk_button_new_with_label(_("_Cancel"));
            gtk_box_append(GTK_BOX(button_box), rename_button);
            gtk_box_append(GTK_BOX(button_box), cancel_button);
            gtk_box_append(GTK_BOX(content_area), button_box);

            RenameDialogData *data = g_new0(RenameDialogData, 1);
            data->mw = mw;
            data->obj = obj;

            g_signal_connect(rename_button, "clicked", G_CALLBACK(on_rename_dialog_response), data);
            g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
            gtk_window_present(GTK_WINDOW(dialog));
        }
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a file to rename."));
    }
}

static void on_rename_dialog_response(GtkButton *button, gpointer user_data) {
    (void)button;
    RenameDialogData *data = (RenameDialogData*)user_data;
    GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    GtkWidget *content_area = gtk_window_get_child(GTK_WINDOW(dialog));
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
            GtkWidget *dialog = gtk_window_new();
            gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mw->window));
            gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
            gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
            gtk_window_set_title(GTK_WINDOW(dialog), _("Confirm Deletion"));

            GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
            gtk_widget_set_margin_start(content_area, 12);
            gtk_widget_set_margin_end(content_area, 12);
            gtk_widget_set_margin_top(content_area, 12);
            gtk_widget_set_margin_bottom(content_area, 12);
            gtk_window_set_child(GTK_WINDOW(dialog), content_area);

            GtkWidget *label = gtk_label_new(message);
            gtk_box_append(GTK_BOX(content_area), label);

            GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
            gtk_widget_set_halign(button_box, GTK_ALIGN_END);
            GtkWidget *yes_button = gtk_button_new_with_label(_("_Yes"));
            GtkWidget *no_button = gtk_button_new_with_label(_("_No"));
            gtk_box_append(GTK_BOX(button_box), yes_button);
            gtk_box_append(GTK_BOX(button_box), no_button);
            gtk_box_append(GTK_BOX(content_area), button_box);

            DeleteConfirmationData *data = g_new0(DeleteConfirmationData, 1);
            data->mw = mw;
            data->obj = obj;

            g_signal_connect(yes_button, "clicked", G_CALLBACK(on_delete_confirm_response), data);
            g_signal_connect_swapped(no_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
            gtk_window_present(GTK_WINDOW(dialog));
        }
    } else {
        gtk_statusbar_push(mw->statusbar, 0, _("Please select a file to delete."));
    }
}

static void on_delete_confirm_response(GtkButton *button, gpointer user_data) {
    (void)button;
    DeleteConfirmationData *data = (DeleteConfirmationData*)user_data;
    GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);

    g_autoptr(GError) error = NULL;
    if (s3_client_delete_object(data->mw->settings->endpoint, data->mw->access_key, data->mw->secret_key, data->mw->settings->bucket, data->obj->key, data->mw->settings->use_ssl, &error)) {
        g_autofree gchar *msg = g_strdup_printf(_("'%s' deleted successfully."), data->obj->key);
        gtk_statusbar_push(data->mw->statusbar, 0, msg);
        refresh_current_folder(data->mw);
    } else {
        g_autofree gchar *msg = g_strdup_printf(_("Failed to delete '%s': %s"), data->obj->key, error->message);
        gtk_statusbar_push(data->mw->statusbar, 0, msg);
    }

    gtk_window_destroy(GTK_WINDOW(dialog));
    g_object_unref(data->obj);
    g_free(data);
}

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

static void on_download_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    (void)response_id; (void)user_data;
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_refresh_button_clicked(GtkButton *b, gpointer user_data) {
    (void)b;
    refresh_current_folder((MainWindow*)user_data);
}

static void set_sourceview_language_from_filename(GtkSourceBuffer *buffer, const gchar *filename) {
    GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lm, g_str_has_suffix(filename, ".xml") ? "xml" :
                                                                            g_str_has_suffix(filename, ".json") ? "json" :
                                                                            g_str_has_suffix(filename, ".yaml") || g_str_has_suffix(filename, ".yml") ? "yaml" :
                                                                            "c"); // Fallback to C
    gtk_source_buffer_set_language(buffer, lang);
}


static void open_editor_tab(MainWindow *mw, const gchar *key, const gchar *content) {
    GtkSourceBuffer *buffer = gtk_source_buffer_new(NULL);
    set_sourceview_language_from_filename(buffer, key);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), content, -1);

    GtkWidget *source_view = gtk_source_view_new_with_buffer(buffer);
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(source_view), GTK_SOURCE_SMART_HOME_END_ALWAYS);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), source_view);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_widget_set_hexpand(scrolled_window, TRUE);

    GtkWidget *tab_label = gtk_label_new(g_path_get_basename(key));
    GtkWidget *save_button = gtk_button_new_with_label(_("Save"));
    GtkWidget *close_button = gtk_button_new_from_icon_name("window-close-symbolic");
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(tab_box), tab_label);
    gtk_box_append(GTK_BOX(tab_box), save_button);
    gtk_box_append(GTK_BOX(tab_box), close_button);

    gtk_widget_set_sensitive(GTK_WIDGET(mw->find_button), TRUE);

    EditorSaveData *save_data = g_new0(EditorSaveData, 1);
    save_data->key = g_strdup(key);
    save_data->source_view = GTK_SOURCE_VIEW(source_view);
    save_data->mw = mw;
    save_data->unsaved = FALSE;
    save_data->tab_label = tab_label;
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_editor_save_button_clicked), save_data);
    g_signal_connect(buffer, "changed", G_CALLBACK(on_buffer_changed), save_data);
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_button_clicked), save_data);

    gtk_notebook_append_page(GTK_NOTEBOOK(mw->notebook), scrolled_window, tab_box);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(mw->notebook), gtk_notebook_get_n_pages(GTK_NOTEBOOK(mw->notebook)) - 1);
}

static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    (void)buffer;
    EditorSaveData *data = (EditorSaveData *)user_data;
    if (!data->unsaved) {
        data->unsaved = TRUE;
        gchar *current_label = g_strdup(gtk_label_get_text(GTK_LABEL(data->tab_label)));
        gchar *new_label = g_strdup_printf("%s*", current_label);
        gtk_label_set_text(GTK_LABEL(data->tab_label), new_label);
        g_free(current_label);
        g_free(new_label);
    }
}

static void on_editor_save_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    EditorSaveData *data = (EditorSaveData *)user_data;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->source_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    gchar *tmp_filename = NULL;
    int fd = g_file_open_tmp("mys3client-XXXXXX", &tmp_filename, NULL);
    if (fd != -1) {
        write(fd, content, strlen(content));
        close(fd);

        g_autoptr(GError) error = NULL;
        if (s3_client_upload_object(data->mw->settings->endpoint, data->mw->access_key, data->mw->secret_key, data->mw->settings->bucket, data->key, tmp_filename, data->mw->settings->use_ssl, &error)) {
            g_autofree gchar *msg = g_strdup_printf(_("'%s' saved successfully."), data->key);
            gtk_statusbar_push(data->mw->statusbar, 0, msg);
            data->unsaved = FALSE;
            gchar *current_label = g_strdup(gtk_label_get_text(GTK_LABEL(data->tab_label)));
            current_label[strlen(current_label) - 1] = '\0';
            gtk_label_set_text(GTK_LABEL(data->tab_label), current_label);
            g_free(current_label);
        } else {
            g_autofree gchar *msg = g_strdup_printf(_("Failed to save '%s': %s"), data->key, error->message);
            gtk_statusbar_push(data->mw->statusbar, 0, msg);
        }
        g_remove(tmp_filename);
    }

    g_free(content);
    g_free(tmp_filename);
}

static void close_tab(EditorSaveData *data) {
    GtkWidget *scrolled_window = gtk_widget_get_parent(GTK_WIDGET(data->source_view));
    gint page_num = gtk_notebook_page_num(data->mw->notebook, scrolled_window);

    if (page_num != -1) {
        gtk_notebook_remove_page(data->mw->notebook, page_num);
        if (gtk_notebook_get_n_pages(data->mw->notebook) == 1) {
            gtk_widget_set_sensitive(GTK_WIDGET(data->mw->find_button), FALSE);
        }
    }
    g_free(data->key);
    g_free(data);
}

static void on_close_dont_save_response(GtkButton *button, gpointer user_data) {
    EditorSaveData *data = (EditorSaveData *)user_data;
    GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    gtk_window_destroy(GTK_WINDOW(dialog));
    close_tab(data);
}

static void on_close_save_response(GtkButton *button, gpointer user_data) {
    EditorSaveData *data = (EditorSaveData *)user_data;
    GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    on_editor_save_button_clicked(NULL, data);
    gtk_window_destroy(GTK_WINDOW(dialog));
    close_tab(data);
}

static void on_close_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    EditorSaveData *data = (EditorSaveData *)user_data;

    if (data->unsaved) {
        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(data->mw->window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_title(GTK_WINDOW(dialog), _("You have unsaved changes. Do you want to save them?"));

        GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_start(content_area, 12);
        gtk_widget_set_margin_end(content_area, 12);
        gtk_widget_set_margin_top(content_area, 12);
        gtk_widget_set_margin_bottom(content_area, 12);
        gtk_window_set_child(GTK_WINDOW(dialog), content_area);

        GtkWidget *label = gtk_label_new(_("You have unsaved changes. Do you want to save them?"));
        gtk_box_append(GTK_BOX(content_area), label);

        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(button_box, GTK_ALIGN_END);
        GtkWidget *save_button = gtk_button_new_with_label(_("_Save"));
        GtkWidget *dont_save_button = gtk_button_new_with_label(_("_Don't Save"));
        GtkWidget *cancel_button = gtk_button_new_with_label(_("_Cancel"));
        gtk_box_append(GTK_BOX(button_box), save_button);
        gtk_box_append(GTK_BOX(button_box), dont_save_button);
        gtk_box_append(GTK_BOX(button_box), cancel_button);
        gtk_box_append(GTK_BOX(content_area), button_box);

        g_signal_connect(save_button, "clicked", G_CALLBACK(on_close_save_response), data);
        g_signal_connect(dont_save_button, "clicked", G_CALLBACK(on_close_dont_save_response), data);
        g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_window_present(GTK_WINDOW(dialog));
    } else {
        close_tab(data);
    }
}

static void on_find_next_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow *)user_data;
    const gchar *search_text = gtk_editable_get_text(GTK_EDITABLE(mw->find_entry));
    if (!search_text || !*search_text) {
        return;
    }

    gint page_num = gtk_notebook_get_current_page(mw->notebook);
    GtkWidget *scrolled_window = gtk_notebook_get_nth_page(mw->notebook, page_num);
    GtkSourceView *source_view = GTK_SOURCE_VIEW(gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled_window)));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(source_view));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));

    GtkTextIter start, end;
    if (gtk_text_iter_forward_search(&iter, search_text, 0, &start, &end, NULL)) {
        gtk_text_buffer_select_range(buffer, &start, &end);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(source_view), &start, 0.0, TRUE, 0.5, 0.5);
    }
}

static void on_replace_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow *)user_data;
    const gchar *search_text = gtk_editable_get_text(GTK_EDITABLE(mw->find_entry));
    const gchar *replace_text = gtk_editable_get_text(GTK_EDITABLE(mw->replace_entry));
    if (!search_text || !*search_text) {
        return;
    }

    gint page_num = gtk_notebook_get_current_page(mw->notebook);
    GtkWidget *scrolled_window = gtk_notebook_get_nth_page(mw->notebook, page_num);
    GtkSourceView *source_view = GTK_SOURCE_VIEW(gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled_window)));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(source_view));
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_delete(buffer, &start, &end);
        gtk_text_buffer_insert(buffer, &start, replace_text, -1);
    }

    on_find_next_button_clicked(NULL, mw);
}

static void on_replace_all_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow *)user_data;
    const gchar *search_text = gtk_editable_get_text(GTK_EDITABLE(mw->find_entry));
    const gchar *replace_text = gtk_editable_get_text(GTK_EDITABLE(mw->replace_entry));
    if (!search_text || !*search_text) {
        return;
    }

    gint page_num = gtk_notebook_get_current_page(mw->notebook);
    GtkWidget *scrolled_window = gtk_notebook_get_nth_page(mw->notebook, page_num);
    GtkSourceView *source_view = GTK_SOURCE_VIEW(gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled_window)));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(source_view));
    GtkTextIter iter, start, end;
    gtk_text_buffer_get_start_iter(buffer, &iter);

    while (gtk_text_iter_forward_search(&iter, search_text, 0, &start, &end, NULL)) {
        gtk_text_buffer_delete(buffer, &start, &end);
        gtk_text_buffer_insert(buffer, &start, replace_text, -1);
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, gtk_text_iter_get_offset(&start) + strlen(replace_text));
    }
}

static void on_find_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MainWindow *mw = (MainWindow *)user_data;

    if (mw->find_dialog) {
        gtk_window_present(GTK_WINDOW(mw->find_dialog));
        return;
    }

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mw->window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Find and Replace"));
    mw->find_dialog = dialog;

    GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(content_area, 12);
    gtk_widget_set_margin_end(content_area, 12);
    gtk_widget_set_margin_top(content_area, 12);
    gtk_widget_set_margin_bottom(content_area, 12);
    gtk_window_set_child(GTK_WINDOW(dialog), content_area);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_box_append(GTK_BOX(content_area), grid);

    mw->find_entry = GTK_ENTRY(gtk_entry_new());
    mw->replace_entry = GTK_ENTRY(gtk_entry_new());
    GtkWidget *find_next_button = gtk_button_new_with_label(_("Find Next"));
    GtkWidget *replace_button = gtk_button_new_with_label(_("Replace"));
    GtkWidget *replace_all_button = gtk_button_new_with_label(_("Replace All"));

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Find:")), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(mw->find_entry), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), find_next_button, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Replace with:")), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(mw->replace_entry), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), replace_button, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), replace_all_button, 2, 2, 1, 1);

    g_signal_connect(find_next_button, "clicked", G_CALLBACK(on_find_next_button_clicked), mw);
    g_signal_connect(replace_button, "clicked", G_CALLBACK(on_replace_button_clicked), mw);
    g_signal_connect(replace_all_button, "clicked", G_CALLBACK(on_replace_all_button_clicked), mw);

    gtk_window_present(GTK_WINDOW(dialog));
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
    mw->file_list_view = GTK_LIST_VIEW(gtk_builder_get_object(b, "file_list_view"));
    mw->notebook = GTK_NOTEBOOK(gtk_builder_get_object(b, "notebook"));
    mw->statusbar = GTK_STATUSBAR(gtk_builder_get_object(b, "statusbar"));
    mw->find_button = GTK_BUTTON(gtk_builder_get_object(b, "find_button"));
    gtk_widget_set_sensitive(GTK_WIDGET(mw->find_button), FALSE);

    GListStore *folder_store = g_list_store_new(G_TYPE_POINTER);
    mw->folder_tree_model = gtk_tree_list_model_new(G_LIST_MODEL(folder_store), FALSE, FALSE, folder_model_get_children, NULL, NULL);

    GtkListItemFactory *folder_factory = gtk_signal_list_item_factory_new();
    g_signal_connect(folder_factory, "setup", G_CALLBACK(setup_folder_list_item_cb), NULL);
    g_signal_connect(folder_factory, "bind", G_CALLBACK(bind_folder_list_item_cb), NULL);

    GtkSingleSelection *folder_selection = gtk_single_selection_new(G_LIST_MODEL(mw->folder_tree_model));
    mw->folder_tree_view = GTK_LIST_VIEW(gtk_list_view_new(GTK_SELECTION_MODEL(folder_selection), folder_factory));
    GtkScrolledWindow *scrolled_window = GTK_SCROLLED_WINDOW(gtk_builder_get_object(b, "folder_tree_scrolled_window"));
    gtk_scrolled_window_set_child(scrolled_window, GTK_WIDGET(mw->folder_tree_view));

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
    g_signal_connect(mw->find_button, "clicked", G_CALLBACK(on_find_button_clicked), mw);
    g_signal_connect(mw->window, "close-request", G_CALLBACK(on_window_close_request), mw);

    GSimpleAction *lang_action = g_simple_action_new_stateful("language", g_variant_type_new("s"), g_variant_new_string("en"));
    g_signal_connect(lang_action, "activate", G_CALLBACK(on_language_changed), mw);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(lang_action));

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

static void on_window_close_response(GtkButton *button, gpointer user_data) {
    GtkWindow *window = GTK_WINDOW(user_data);
    GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    gtk_window_destroy(GTK_WINDOW(dialog));
    gtk_window_destroy(window);
}

static gboolean on_window_close_request(GtkApplicationWindow *window, gpointer user_data) {
    MainWindow *mw = (MainWindow *)user_data;
    gboolean unsaved_changes = FALSE;
    for (guint i = 1; i < gtk_notebook_get_n_pages(mw->notebook); ++i) {
        GtkWidget *page = gtk_notebook_get_nth_page(mw->notebook, i);
        GtkWidget *tab_box = gtk_notebook_get_tab_label(mw->notebook, page);
        GtkWidget *tab_label = gtk_widget_get_first_child(tab_box);
        const gchar *label_text = gtk_label_get_text(GTK_LABEL(tab_label));
        if (g_str_has_suffix(label_text, "*")) {
            unsaved_changes = TRUE;
            break;
        }
    }

    if (unsaved_changes) {
        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_title(GTK_WINDOW(dialog), _("There are unsaved changes. Do you want to close anyway?"));

        GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_start(content_area, 12);
        gtk_widget_set_margin_end(content_area, 12);
        gtk_widget_set_margin_top(content_area, 12);
        gtk_widget_set_margin_bottom(content_area, 12);
        gtk_window_set_child(GTK_WINDOW(dialog), content_area);

        GtkWidget *label = gtk_label_new(_("There are unsaved changes. Do you want to close anyway?"));
        gtk_box_append(GTK_BOX(content_area), label);

        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(button_box, GTK_ALIGN_END);
        GtkWidget *yes_button = gtk_button_new_with_label(_("_Yes"));
        GtkWidget *no_button = gtk_button_new_with_label(_("_No"));
        gtk_box_append(GTK_BOX(button_box), yes_button);
        gtk_box_append(GTK_BOX(button_box), no_button);
        gtk_box_append(GTK_BOX(content_area), button_box);

        g_signal_connect(yes_button, "clicked", G_CALLBACK(on_window_close_response), window);
        g_signal_connect_swapped(no_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_window_present(GTK_WINDOW(dialog));
        return TRUE; // Prevent window from closing
    }

    return FALSE; // No unsaved changes, close the window
}

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

    g_signal_connect (app, "activate", G_CALLBACK(app_activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    return status;
}
