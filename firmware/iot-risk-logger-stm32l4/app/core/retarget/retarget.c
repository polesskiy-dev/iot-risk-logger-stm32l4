/*!
 * @file retarget.c
 * @brief implementation of retarget
 *
 * Detailed description of the implementation file.
 *
 * @date 13/08/2024
 * @author artempolisskyi
 */

#include "retarget.h"

static int retargetWriteInfo(struct _reent *r, void *fd, const char *ptr, int len);
static int retargetWriteError(struct _reent *r, void *fd, const char *ptr, int len);

void RETARGET_Init(void) {
  // Redirect stdout and stderr to SEGGER SystemView
  setvbuf(stdout, NULL, _IONBF, 0); // No buffering for stdout
  setvbuf(stderr, NULL, _IONBF, 0); // No buffering for stderr

  stdout->_write = retargetWriteInfo;
  stderr->_write = retargetWriteError;
}

static int retargetWriteInfo(struct _reent *r, void *fd, const char *ptr, int len)
{
  (void)r; // Unused parameter
  int i;

  for (i = 0; i < len; i++) {
    SEGGER_SYSVIEW_Print(ptr + i);
  }

  return len;  // Return the number of bytes written
}

static int retargetWriteError(struct _reent *r, void *fd, const char *ptr, int len)
{
  (void)r; // Unused parameter
  int i;

  for (i = 0; i < len; i++) {
    SEGGER_SYSVIEW_Error(ptr + i);
  }

  return len;  // Return the number of bytes written
}