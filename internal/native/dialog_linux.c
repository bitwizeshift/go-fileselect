#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dialog.h"

/* dup_str duplicates a C string onto the heap; NULL becomes empty. */
static char *dup_str(const char *s) { return strdup(s ? s : ""); }

/* gtk_action maps a dialog mode onto the matching GtkFileChooserAction. */
static GtkFileChooserAction gtk_action(int mode) {
  switch (mode) {
  case FILESELECT_MODE_SAVE_FILE:
    return GTK_FILE_CHOOSER_ACTION_SAVE;
  case FILESELECT_MODE_OPEN_DIR:
    return GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  default:
    return GTK_FILE_CHOOSER_ACTION_OPEN;
  }
}

/* add_filters attaches one GtkFileFilter per named filter group. */
static void add_filters(GtkFileChooser *chooser, const char **filter_names,
                        const char **filter_specs, int filter_count) {
  for (int i = 0; i < filter_count; i++) {
    GtkFileFilter *filter = gtk_file_filter_new();
    const char *name = (filter_names && filter_names[i]) ? filter_names[i] : "";
    gtk_file_filter_set_name(filter, name[0] != '\0' ? name : "Files");

    const char *spec = filter_specs ? filter_specs[i] : NULL;
    if (spec == NULL || spec[0] == '\0') {
      gtk_file_filter_add_pattern(filter, "*");
    } else {
      char *copy = strdup(spec);
      char *save = NULL;
      for (char *tok = strtok_r(copy, ";", &save); tok != NULL;
           tok = strtok_r(NULL, ";", &save)) {
        char pattern[256];
        snprintf(pattern, sizeof(pattern), "*.%s", tok);
        gtk_file_filter_add_pattern(filter, pattern);
      }
      free(copy);
    }
    gtk_file_chooser_add_filter(chooser, filter);
  }
}

/* drain_events processes pending GTK events so the dialog window is destroyed
 * before control returns to the caller. */
static void drain_events(void) {
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }
}

/* collect appends a single absolute path to the result. */
static void append_path(fileselect_result *result, char *path) {
  result->paths =
      realloc(result->paths, (size_t)(result->count + 1) * sizeof(char *));
  result->paths[result->count++] = path;
}

fileselect_result fileselect_run(int mode, const char *title, const char *dir,
                                 const char *filename,
                                 const char **filter_names,
                                 const char **filter_specs, int filter_count) {
  fileselect_result result = {FILESELECT_CANCELLED, 0, NULL, NULL};

  if (!gtk_init_check(NULL, NULL)) {
    result.status = FILESELECT_ERROR;
    result.error = dup_str("GTK could not be initialized");
    return result;
  }

  int saving = (mode == FILESELECT_MODE_SAVE_FILE);
  const char *accept = saving ? "_Save" : "_Open";
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      (title && title[0] != '\0') ? title : NULL, NULL, gtk_action(mode),
      "_Cancel", GTK_RESPONSE_CANCEL, accept, GTK_RESPONSE_ACCEPT, NULL);

  GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
  gtk_file_chooser_set_select_multiple(chooser,
                                       mode == FILESELECT_MODE_OPEN_FILES);
  if (saving) {
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
  }
  if (dir && dir[0] != '\0') {
    gtk_file_chooser_set_current_folder(chooser, dir);
  }
  if (filename && filename[0] != '\0') {
    gtk_file_chooser_set_current_name(chooser, filename);
  }
  if (mode != FILESELECT_MODE_OPEN_DIR) {
    add_filters(chooser, filter_names, filter_specs, filter_count);
  }

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    result.status = FILESELECT_OK;
    GSList *files = gtk_file_chooser_get_filenames(chooser);
    for (GSList *node = files; node != NULL; node = node->next) {
      append_path(&result, dup_str((const char *)node->data));
      g_free(node->data);
    }
    g_slist_free(files);
  }

  gtk_widget_destroy(dialog);
  drain_events();
  return result;
}

void fileselect_free(fileselect_result result) {
  for (int i = 0; i < result.count; i++) {
    free(result.paths[i]);
  }
  free(result.paths);
  free(result.error);
}
