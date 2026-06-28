#include "dialog.h"

#import <AppKit/AppKit.h>
#include <stdlib.h>
#include <string.h>

/* fileselect_copy duplicates a C string onto the heap; NULL becomes empty. */
static char *fileselect_copy(const char *s) { return strdup(s ? s : ""); }

/* fileselect_extensions splits ';'-separated specs into an NSArray of bare
 * extensions, deduplicated across all filter groups (NSOpenPanel has no notion
 * of named filter groups, so they are flattened). */
static NSArray<NSString *> *fileselect_extensions(const char **filter_specs,
                                                  int filter_count) {
  NSMutableArray<NSString *> *types = [NSMutableArray array];
  for (int i = 0; i < filter_count; i++) {
    if (filter_specs[i] == NULL) {
      continue;
    }
    NSString *spec = [NSString stringWithUTF8String:filter_specs[i]];
    for (NSString *ext in [spec componentsSeparatedByString:@";"]) {
      if (ext.length > 0 && ![types containsObject:ext]) {
        [types addObject:ext];
      }
    }
  }
  return types.count > 0 ? types : nil;
}

/* fileselect_make_panel builds the open or save panel for the given mode. */
static NSSavePanel *fileselect_make_panel(int mode) {
  if (mode == FILESELECT_MODE_SAVE_FILE) {
    return [NSSavePanel savePanel];
  }
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  panel.canChooseFiles = (mode != FILESELECT_MODE_OPEN_DIR);
  panel.canChooseDirectories = (mode == FILESELECT_MODE_OPEN_DIR);
  panel.allowsMultipleSelection = (mode == FILESELECT_MODE_OPEN_FILES);
  return panel;
}

/* fileselect_collect copies the panel's selected paths into result. */
static void fileselect_collect(NSSavePanel *panel, int mode,
                               fileselect_result *result) {
  NSArray<NSURL *> *urls;
  if (mode == FILESELECT_MODE_SAVE_FILE) {
    urls = panel.URL ? @[ panel.URL ] : @[];
  } else {
    urls = ((NSOpenPanel *)panel).URLs;
  }

  result->count = (int)urls.count;
  if (result->count == 0) {
    return;
  }
  result->paths = calloc((size_t)result->count, sizeof(char *));
  for (int i = 0; i < result->count; i++) {
    result->paths[i] = fileselect_copy(urls[i].path.UTF8String);
  }
}

static fileselect_result fileselect_present(int mode, const char *title,
                                            const char *dir,
                                            const char *filename,
                                            const char **filter_specs,
                                            int filter_count) {
  fileselect_result result = {FILESELECT_CANCELLED, 0, NULL, NULL};
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    [NSApp activateIgnoringOtherApps:YES];

    NSSavePanel *panel = fileselect_make_panel(mode);
    if (title && title[0] != '\0') {
      panel.title = [NSString stringWithUTF8String:title];
    }
    if (dir && dir[0] != '\0') {
      panel.directoryURL =
          [NSURL fileURLWithPath:[NSString stringWithUTF8String:dir]];
    }
    if (filename && filename[0] != '\0') {
      panel.nameFieldStringValue = [NSString stringWithUTF8String:filename];
    }
    if (mode != FILESELECT_MODE_OPEN_DIR) {
      NSArray<NSString *> *types =
          fileselect_extensions(filter_specs, filter_count);
      if (types != nil) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        panel.allowedFileTypes = types;
#pragma clang diagnostic pop
      }
    }

    if ([panel runModal] == NSModalResponseOK) {
      result.status = FILESELECT_OK;
      fileselect_collect(panel, mode, &result);
    }
  }
  return result;
}

fileselect_result fileselect_run(int mode, const char *title, const char *dir,
                                 const char *filename,
                                 const char **filter_names,
                                 const char **filter_specs, int filter_count) {
  (void)filter_names; /* names are unused on macOS; types are flattened */

  /* AppKit requires panel presentation on the main thread. */
  if ([NSThread isMainThread]) {
    return fileselect_present(mode, title, dir, filename, filter_specs,
                              filter_count);
  }

  __block fileselect_result result;
  dispatch_sync(dispatch_get_main_queue(), ^{
    result = fileselect_present(mode, title, dir, filename, filter_specs,
                                filter_count);
  });
  return result;
}

void fileselect_free(fileselect_result result) {
  for (int i = 0; i < result.count; i++) {
    free(result.paths[i]);
  }
  free(result.paths);
  free(result.error);
}
