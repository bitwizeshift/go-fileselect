#define COBJMACROS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dialog.h"

/* to_wide converts a UTF-8 string to a heap-allocated UTF-16 string. */
static wchar_t *to_wide(const char *s) {
  if (s == NULL) {
    return NULL;
  }
  int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  if (n <= 0) {
    return NULL;
  }
  wchar_t *w = malloc((size_t)n * sizeof(wchar_t));
  if (w != NULL) {
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
  }
  return w;
}

/* to_utf8 converts a UTF-16 string to a heap-allocated UTF-8 string. */
static char *to_utf8(const wchar_t *w) {
  if (w == NULL) {
    return NULL;
  }
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
  if (n <= 0) {
    return NULL;
  }
  char *s = malloc((size_t)n);
  if (s != NULL) {
    WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL);
  }
  return s;
}

/* build_pattern turns a ';'-separated extension list ("bin;cue") into a Win32
 * filter pattern ("*.bin;*.cue"). An empty spec matches all files. */
static char *build_pattern(const char *spec) {
  if (spec == NULL || spec[0] == '\0') {
    return strdup("*.*");
  }
  char *out = malloc(strlen(spec) * 3 + 16);
  if (out == NULL) {
    return NULL;
  }
  out[0] = '\0';

  char *copy = strdup(spec);
  char *ctx = NULL;
  int first = 1;
  for (char *tok = strtok_s(copy, ";", &ctx); tok != NULL;
       tok = strtok_s(NULL, ";", &ctx)) {
    if (!first) {
      strcat(out, ";");
    }
    strcat(out, "*.");
    strcat(out, tok);
    first = 0;
  }
  free(copy);
  if (first) { /* spec held only separators */
    strcpy(out, "*.*");
  }
  return out;
}

/* fileselect_error builds an error result carrying a formatted HRESULT. */
static fileselect_result fileselect_error(HRESULT hr) {
  fileselect_result result = {FILESELECT_ERROR, 0, NULL, NULL};
  char *msg = malloc(64);
  if (msg != NULL) {
    snprintf(msg, 64, "dialog failed (hr=0x%08lx)", (unsigned long)hr);
  }
  result.error = msg;
  return result;
}

/* item_path returns the filesystem path of a shell item as UTF-8. */
static char *item_path(IShellItem *item) {
  PWSTR wpath = NULL;
  if (FAILED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &wpath))) {
    return NULL;
  }
  char *path = to_utf8(wpath);
  CoTaskMemFree(wpath);
  return path;
}

/* configure_options applies the mode-specific dialog flags. */
static void configure_options(IFileDialog *dialog, int mode) {
  DWORD opts = 0;
  IFileDialog_GetOptions(dialog, &opts);
  opts |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
  switch (mode) {
  case FILESELECT_MODE_OPEN_FILE:
    opts |= FOS_FILEMUSTEXIST;
    break;
  case FILESELECT_MODE_OPEN_FILES:
    opts |= FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT;
    break;
  case FILESELECT_MODE_OPEN_DIR:
    opts |= FOS_PICKFOLDERS;
    break;
  case FILESELECT_MODE_SAVE_FILE:
    opts |= FOS_OVERWRITEPROMPT;
    break;
  }
  IFileDialog_SetOptions(dialog, opts);
}

/* apply_filters builds and assigns the COMDLG_FILTERSPEC array. */
static void apply_filters(IFileDialog *dialog, const char **filter_names,
                          const char **filter_specs, int filter_count) {
  if (filter_count <= 0) {
    return;
  }
  COMDLG_FILTERSPEC *specs = calloc((size_t)filter_count, sizeof(*specs));
  wchar_t **names_w = calloc((size_t)filter_count, sizeof(wchar_t *));
  wchar_t **specs_w = calloc((size_t)filter_count, sizeof(wchar_t *));
  if (specs == NULL || names_w == NULL || specs_w == NULL) {
    goto cleanup;
  }
  for (int i = 0; i < filter_count; i++) {
    const char *name = (filter_names && filter_names[i]) ? filter_names[i] : "";
    char *pattern = build_pattern(filter_specs ? filter_specs[i] : NULL);
    names_w[i] = to_wide(name[0] != '\0' ? name : "Files");
    specs_w[i] = to_wide(pattern);
    free(pattern);
    specs[i].pszName = names_w[i];
    specs[i].pszSpec = specs_w[i];
  }
  IFileDialog_SetFileTypes(dialog, (UINT)filter_count, specs);
  IFileDialog_SetFileTypeIndex(dialog, 1);

cleanup:
  for (int i = 0; names_w && i < filter_count; i++) {
    free(names_w[i]);
  }
  for (int i = 0; specs_w && i < filter_count; i++) {
    free(specs_w[i]);
  }
  free(names_w);
  free(specs_w);
  free(specs);
}

/* apply_location sets the initial folder, suggested filename, and title. */
static void apply_location(IFileDialog *dialog, const char *title,
                           const char *dir, const char *filename) {
  if (title && title[0] != '\0') {
    wchar_t *w = to_wide(title);
    IFileDialog_SetTitle(dialog, w);
    free(w);
  }
  if (dir && dir[0] != '\0') {
    wchar_t *w = to_wide(dir);
    IShellItem *folder = NULL;
    if (w && SUCCEEDED(SHCreateItemFromParsingName(w, NULL, &IID_IShellItem,
                                                   (void **)&folder))) {
      IFileDialog_SetFolder(dialog, folder);
      IShellItem_Release(folder);
    }
    free(w);
  }
  if (filename && filename[0] != '\0') {
    wchar_t *w = to_wide(filename);
    IFileDialog_SetFileName(dialog, w);
    free(w);
  }
}

/* collect_multi reads every selected path from a multi-select open dialog. */
static void collect_multi(IFileOpenDialog *dialog, fileselect_result *result) {
  IShellItemArray *items = NULL;
  if (FAILED(IFileOpenDialog_GetResults(dialog, &items))) {
    return;
  }
  DWORD count = 0;
  IShellItemArray_GetCount(items, &count);
  if (count > 0) {
    result->paths = calloc(count, sizeof(char *));
    for (DWORD i = 0; i < count; i++) {
      IShellItem *item = NULL;
      if (SUCCEEDED(IShellItemArray_GetItemAt(items, i, &item))) {
        result->paths[result->count++] = item_path(item);
        IShellItem_Release(item);
      }
    }
  }
  IShellItemArray_Release(items);
}

/* collect_single reads the single selected path from a dialog. */
static void collect_single(IFileDialog *dialog, fileselect_result *result) {
  IShellItem *item = NULL;
  if (FAILED(IFileDialog_GetResult(dialog, &item))) {
    return;
  }
  result->paths = calloc(1, sizeof(char *));
  result->paths[result->count++] = item_path(item);
  IShellItem_Release(item);
}

fileselect_result fileselect_run(int mode, const char *title, const char *dir,
                                 const char *filename,
                                 const char **filter_names,
                                 const char **filter_specs, int filter_count) {
  fileselect_result result = {FILESELECT_CANCELLED, 0, NULL, NULL};

  HRESULT hrInit = CoInitializeEx(
      NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  int owns_com = SUCCEEDED(hrInit);

  int saving = (mode == FILESELECT_MODE_SAVE_FILE);
  IFileDialog *dialog = NULL;
  HRESULT hr = CoCreateInstance(
      saving ? &CLSID_FileSaveDialog : &CLSID_FileOpenDialog, NULL,
      CLSCTX_INPROC_SERVER,
      saving ? &IID_IFileSaveDialog : &IID_IFileOpenDialog, (void **)&dialog);
  if (FAILED(hr)) {
    result = fileselect_error(hr);
    goto done;
  }

  configure_options(dialog, mode);
  apply_location(dialog, title, dir, filename);
  if (mode != FILESELECT_MODE_OPEN_DIR) {
    apply_filters(dialog, filter_names, filter_specs, filter_count);
  }

  hr = IFileDialog_Show(dialog, NULL);
  if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
    result.status = FILESELECT_CANCELLED;
  } else if (FAILED(hr)) {
    result = fileselect_error(hr);
  } else {
    result.status = FILESELECT_OK;
    if (mode == FILESELECT_MODE_OPEN_FILES) {
      collect_multi((IFileOpenDialog *)dialog, &result);
    } else {
      collect_single(dialog, &result);
    }
  }

  IFileDialog_Release(dialog);

done:
  if (owns_com) {
    CoUninitialize();
  }
  return result;
}

void fileselect_free(fileselect_result result) {
  for (int i = 0; i < result.count; i++) {
    free(result.paths[i]);
  }
  free(result.paths);
  free(result.error);
}
