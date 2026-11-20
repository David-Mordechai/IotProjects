/**
 * @file big_static.h
 * @brief Large static buffers that live on the heap on the classic ESP32.
 *
 * The classic ESP32 places all static (.bss/.data) variables in the
 * dram0_0_seg region, which is much smaller than the chip's total RAM.
 * WireClaw's big buffers overflow it. On that chip BIG_STATIC allocates the
 * buffer from the heap on first use instead; the name is a reference to the
 * same type, so sizeof() and indexing behave exactly as before. Memory is
 * zeroed, like a normal static. Other targets keep a plain static.
 *
 * Usage:  BIG_STATIC(char[4096], buf);   BIG_STATIC(LlmResult, result);
 */
#ifndef BIG_STATIC_H
#define BIG_STATIC_H

#include <stdlib.h>
#include <sdkconfig.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_rom_sys.h>

template <typename T> using BigStaticType = T;

template <typename T>
static inline T &bigStaticAlloc(const char *name) {
    void *p = heap_caps_calloc(1, sizeof(T), MALLOC_CAP_8BIT);
    if (!p) {
        esp_rom_printf("[BIG_STATIC] out of memory: %s (%u bytes, largest free block %u)\n",
                       name, (unsigned)sizeof(T),
                       (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        abort();
    }
    return *static_cast<T *>(p);
}

/* Heap buffer owned by the enclosing scope: allocated (zeroed) where it is
 * declared, freed when the scope ends (RAII). */
template <typename T>
struct BigTemp {
    T *p;
    explicit BigTemp(const char *name) : p(&bigStaticAlloc<T>(name)) {}
    ~BigTemp() { heap_caps_free(p); }
    BigTemp(const BigTemp &) = delete;
    BigTemp &operator=(const BigTemp &) = delete;
};

#if CONFIG_IDF_TARGET_ESP32
#define BIG_STATIC(T, name) \
    static BigStaticType<T> &name = bigStaticAlloc<BigStaticType<T>>(#name)
/* BIG_TEMP: for buffers used only during one call. Unlike BIG_STATIC the
 * memory goes back to the heap when the function returns. This matters: on
 * the classic ESP32, TCP sends larger than one lwIP send buffer (5744 B)
 * failed (errno 11, stalled) once free heap fell below ~64-68 KB, measured
 * 2026-11-20 (see PORT-NOTES.md). Keeping every buffer allocated forever
 * left only ~70 KB free after the first chat. */
#define BIG_TEMP(T, name) \
    BigTemp<BigStaticType<T>> name##_heap_(#name); \
    BigStaticType<T> &name = *name##_heap_.p
#else
#define BIG_STATIC(T, name) static BigStaticType<T> name
#define BIG_TEMP(T, name)   static BigStaticType<T> name
#endif

#endif /* BIG_STATIC_H */
