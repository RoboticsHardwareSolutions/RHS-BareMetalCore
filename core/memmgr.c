#include "memmgr.h"
#include "check.h"
#include "log.h"
#include <string.h>
#include "FreeRTOS.h"

extern void*  pvPortMalloc(size_t xSize);
extern void   vPortFree(void* pv);
extern size_t xPortGetFreeHeapSize(void);
extern size_t xPortGetMinimumEverFreeHeapSize(void);

void* malloc(size_t size)
{
    void* p = pvPortMalloc(size);
    return p;
}

void free(void* ptr)
{
    vPortFree(ptr);
}

void* realloc(void* ptr, size_t size)
{
    if (size == 0)
    {
        vPortFree(ptr);
        return NULL;
    }

    void* p = pvPortMalloc(size);
    if (ptr != NULL)
    {
        memcpy(p, ptr, size);
        vPortFree(ptr);
    }

    return p;
}

void* calloc(size_t nmemb, size_t size)
{
    void* p = pvPortMalloc(nmemb * size);
    if (p != NULL)
    {
        memset(p, 0, nmemb * size);
    }
    return p;
}

char* strdup(const char* s)
{
    rhs_assert(s != NULL);

    size_t siz = strlen(s) + 1;
    char*  y   = malloc(siz);
    memcpy(y, s, siz);

    return y;
}

size_t memmgr_get_free_heap(void)
{
    return xPortGetFreeHeapSize();
}

size_t memmgr_get_total_heap(void)
{
    return configTOTAL_HEAP_SIZE;
}

size_t memmgr_get_minimum_free_heap(void)
{
    return xPortGetMinimumEverFreeHeapSize();
}
