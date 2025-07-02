#include "loader.h"
#include "loader_i.h"

#define TAG "loader"

static Loader* loader_alloc(void)
{
    Loader* loader = malloc(sizeof(Loader));
    loader->queue  = rhs_message_queue_alloc(1, sizeof(LoaderMessage));
    return loader;
}

int32_t loader_service(void* context)
{
    Loader* loader = loader_alloc();
    RHS_LOG_I(TAG, "Executing system start hooks");
    for (size_t i = 0; i < RHS_START_UP_COUNT; i++)
    {
        RHS_START_UP[i]();
    }

    LoaderMessage message;
    for (;;)
    {
        if (rhs_message_queue_get(loader->queue, &message, RHSWaitForever) == RHSStatusOk)
        {
        }
    }

    return 0;
}
