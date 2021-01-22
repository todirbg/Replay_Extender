/*

  FILE: DebugPrint.h

  BD-5J Plugin for X-Plane 11
  Copyright © 2019 by Quantumac
  Edited by Todir 2021 - Change DEBUG_PREFIX

  GNU GENERAL PUBLIC LICENSE, Version 2, June 1991

*/

#ifndef __DEBUG_PRINT__
#define __DEBUG_PRINT__

//--------------------------------------------------------------------------------------------------------------------
// CONDITIONALS
//--------------------------------------------------------------------------------------------------------------------
#define COND_PRINT                  1

//--------------------------------------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------------------------------------
#include "XPLMUtilities.h"

//--------------------------------------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------------------------------------
#define DEBUG_PREFIX   "Replay Extender: "

//--------------------------------------------------------------------------------------------------------------------
// Debug Printing
//--------------------------------------------------------------------------------------------------------------------
#if COND_PRINT

#define DPRINT(format, ...) \
{ \
  char __debugPrintBuff[256]; \
  snprintf(__debugPrintBuff, sizeof(__debugPrintBuff), DEBUG_PREFIX format, __VA_ARGS__); \
  XPLMDebugString(__debugPrintBuff); \
}

#define DPUTS(str) XPLMDebugString(DEBUG_PREFIX str)

#else

#define DPRINT(format, args...)
#define DPUTS(format)

#endif

#endif // __DEBUG_PRINT__
