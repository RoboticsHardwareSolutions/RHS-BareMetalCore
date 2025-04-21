#include "stream_buf.h"
#include "common.h"
#include "memmgr.h"
#include "check.h"

#include <FreeRTOS.h>
#include "stream_buffer.h"

// Internal FreeRTOS member names
#define xTriggerLevelBytes uxDummy1[3]

struct RHSStreamBuffer
{
    StaticStreamBuffer_t container;
    uint8_t              buffer[];
};

// IMPORTANT: container MUST be the FIRST struct member
static_assert(offsetof(RHSStreamBuffer, container) == 0);
// IMPORTANT: buffer MUST be the LAST struct member
static_assert(offsetof(RHSStreamBuffer, buffer) == sizeof(RHSStreamBuffer));

RHSStreamBuffer* rhs_stream_buffer_alloc(uint16_t size, uint16_t trigger_level)
{
    rhs_assert(size != 0);

    // Actual FreeRTOS usable buffer size seems to be one less
    const uint16_t buffer_size = size + 1;

    RHSStreamBuffer*     stream_buffer = malloc(sizeof(RHSStreamBuffer) + buffer_size);
    StreamBufferHandle_t hStreamBuffer =
        xStreamBufferCreateStatic(buffer_size, trigger_level, stream_buffer->buffer, &stream_buffer->container);

    rhs_assert(hStreamBuffer == (StreamBufferHandle_t) stream_buffer);

    return stream_buffer;
}

void rhs_stream_buffer_free(RHSStreamBuffer* stream_buffer)
{
    rhs_assert(stream_buffer);
    vStreamBufferDelete((StreamBufferHandle_t) stream_buffer);
    free(stream_buffer);
}

bool rhs_stream_set_trigger_level(RHSStreamBuffer* stream_buffer, uint16_t trigger_level)
{
    rhs_assert(stream_buffer);
    return xStreamBufferSetTriggerLevel((StreamBufferHandle_t) stream_buffer, trigger_level) == pdTRUE;
}

uint16_t rhs_stream_get_trigger_level(RHSStreamBuffer* stream_buffer)
{
    rhs_assert(stream_buffer);
    return ((StaticStreamBuffer_t*) stream_buffer)->xTriggerLevelBytes;
}

uint16_t rhs_stream_buffer_send(RHSStreamBuffer* stream_buffer, const void* data, uint16_t length, uint32_t timeout)
{
    rhs_assert(stream_buffer);

    uint16_t ret;

    if (RHS_IS_IRQ_MODE())
    {
        BaseType_t yield;
        ret = xStreamBufferSendFromISR((StreamBufferHandle_t) stream_buffer, data, length, &yield);
        portYIELD_FROM_ISR(yield);
    }
    else
    {
        ret = xStreamBufferSend((StreamBufferHandle_t) stream_buffer, data, length, timeout);
    }

    return ret;
}

uint16_t rhs_stream_buffer_receive(RHSStreamBuffer* stream_buffer, void* data, uint16_t length, uint32_t timeout)
{
    rhs_assert(stream_buffer);

    uint16_t ret;

    if (RHS_IS_IRQ_MODE())
    {
        BaseType_t yield;
        ret = xStreamBufferReceiveFromISR((StreamBufferHandle_t) stream_buffer, data, length, &yield);
        portYIELD_FROM_ISR(yield);
    }
    else
    {
        ret = xStreamBufferReceive((StreamBufferHandle_t) stream_buffer, data, length, timeout);
    }

    return ret;
}

uint16_t rhs_stream_buffer_bytes_available(RHSStreamBuffer* stream_buffer)
{
    rhs_assert(stream_buffer);

    return xStreamBufferBytesAvailable((StreamBufferHandle_t) stream_buffer);
}

uint16_t rhs_stream_buffer_spaces_available(RHSStreamBuffer* stream_buffer)
{
    rhs_assert(stream_buffer);

    return xStreamBufferSpacesAvailable((StreamBufferHandle_t) stream_buffer);
}

bool rhs_stream_buffer_is_full(RHSStreamBuffer* stream_buffer)
{
    rhs_assert(stream_buffer);

    return xStreamBufferIsFull((StreamBufferHandle_t) stream_buffer) == pdTRUE;
}

bool rhs_stream_buffer_is_empty(RHSStreamBuffer* stream_buffer)
{
    rhs_assert(stream_buffer);

    return xStreamBufferIsEmpty((StreamBufferHandle_t) stream_buffer) == pdTRUE;
}

RHSStatus rhs_stream_buffer_reset(RHSStreamBuffer* stream_buffer)
{
    rhs_assert(stream_buffer);

    RHSStatus status;

    if (xStreamBufferReset((StreamBufferHandle_t) stream_buffer) == pdPASS)
    {
        status = RHSStatusOk;
    }
    else
    {
        status = RHSStatusError;
    }

    return status;
}
