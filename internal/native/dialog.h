#ifndef FILESELECT_DIALOG_H
#define FILESELECT_DIALOG_H

/* Dialog kinds. These mirror the native.Mode constants on the Go side. */
enum fileselect_mode {
  FILESELECT_MODE_OPEN_FILE = 0,
  FILESELECT_MODE_OPEN_FILES = 1,
  FILESELECT_MODE_SAVE_FILE = 2,
  FILESELECT_MODE_OPEN_DIR = 3
};

/* Outcome of presenting a dialog. */
enum fileselect_status {
  FILESELECT_OK = 0,        /* user confirmed a selection */
  FILESELECT_CANCELLED = 1, /* user dismissed the dialog  */
  FILESELECT_ERROR = 2      /* the dialog could not be shown */
};

/*
 * fileselect_result carries the outcome of fileselect_run back to Go.
 *
 * On FILESELECT_OK, paths points to `count` heap-allocated, NUL-terminated
 * UTF-8 strings. On FILESELECT_ERROR, error may hold a heap-allocated message.
 * Ownership transfers to the caller, which must release it with
 * fileselect_free.
 */
typedef struct {
  int status;
  int count;
  char **paths;
  char *error;
} fileselect_result;

/*
 * fileselect_run presents the requested modal dialog and blocks until the user
 * responds.
 *
 * filter_names and filter_specs are parallel arrays of length filter_count.
 * Each filter_specs entry is a ';'-separated list of bare extensions (no
 * leading dot), e.g. "bin;cue". A filter_count of 0 means "all files".
 *
 * Each platform provides its own implementation of this function.
 */
fileselect_result fileselect_run(int mode, const char *title, const char *dir,
                                 const char *filename,
                                 const char **filter_names,
                                 const char **filter_specs, int filter_count);

/* fileselect_free releases the memory owned by a fileselect_result. */
void fileselect_free(fileselect_result result);

#endif /* FILESELECT_DIALOG_H */
