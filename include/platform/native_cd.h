#ifndef NATIVE_CD_H
#define NATIVE_CD_H

// NOTE(aalhendi): Native Cd* facade for extracted assets and disc-image fallback.
int NativeCD_Init(void);
void NativeCD_PumpCallbacks(void);
void NativeCD_Shutdown(void);

#if defined(__SWITCH__)
// NOTE(aalhendi): Diagnostic-only accessor for the CD read worker thread's
// counters. Safe to call from the main thread (takes the worker mutex
// internally); see native_cd.c for why the worker thread itself must never
// call printf directly on Switch.
void NativeCD_DiagGetCounters(int *readsStarted, int *readsFinished, int *lastFileIndex, int *lastSuccess, int *pumpDispatchCount);
#endif

#endif
