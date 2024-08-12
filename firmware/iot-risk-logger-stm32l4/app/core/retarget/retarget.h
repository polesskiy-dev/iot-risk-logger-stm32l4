/*!
 * @file retarget.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 13/08/2024
 * @author artempolisskyi
 */

#ifndef RETARGET_H
#define RETARGET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "SEGGER_SYSVIEW_Conf.h"
#include "SEGGER_SYSVIEW.h"

void RETARGET_Init(void);

#ifdef __cplusplus
}
#endif

#endif //RETARGET_H