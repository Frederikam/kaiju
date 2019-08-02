#pragma once
#ifndef KONAN_LIBKAIJU_BRIDGE_H
#define KONAN_LIBKAIJU_BRIDGE_H

#include "./include/bridge/hooks.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
typedef bool            libkaiju_bridge_KBoolean;
#else
typedef _Bool           libkaiju_bridge_KBoolean;
#endif
typedef unsigned short     libkaiju_bridge_KChar;
typedef signed char        libkaiju_bridge_KByte;
typedef short              libkaiju_bridge_KShort;
typedef int                libkaiju_bridge_KInt;
typedef long long          libkaiju_bridge_KLong;
typedef unsigned char      libkaiju_bridge_KUByte;
typedef unsigned short     libkaiju_bridge_KUShort;
typedef unsigned int       libkaiju_bridge_KUInt;
typedef unsigned long long libkaiju_bridge_KULong;
typedef float              libkaiju_bridge_KFloat;
typedef double             libkaiju_bridge_KDouble;
typedef void*              libkaiju_bridge_KNativePtr;
struct libkaiju_bridge_KType;
typedef struct libkaiju_bridge_KType libkaiju_bridge_KType;



typedef struct {
  /* Service functions. */
  void (*DisposeStablePointer)(libkaiju_bridge_KNativePtr ptr);
  void (*DisposeString)(const char* string);
  libkaiju_bridge_KBoolean (*IsInstance)(libkaiju_bridge_KNativePtr ref, const libkaiju_bridge_KType* type);

  /* User functions. */
  struct {
    struct {
      struct {
        struct {
          struct {
              struct bridge_hooks* (*kaijuEntry)();
          } kaiju;
        } frederikam;
      } com;
    } root;
  } kotlin;
} libkaiju_bridge_ExportedSymbols;
extern libkaiju_bridge_ExportedSymbols* libkaiju_bridge_symbols(void);
#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif  /* KONAN_LIBKAIJU_BRIDGE_H */
