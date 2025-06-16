#include "record.h"
#include "check.h"
#include "mutex.h"
#include "event_flag.h"

#include <m-dict.h>
#include "m_cstr_dup.h"

#define RHS_RECORD_FLAG_READY (0x1)

typedef struct {
    RHSEventFlag* flags;
    void* data;
    size_t holders_count;
} RHSRecordData;

DICT_DEF2(RHSRecordDataDict, const char*, M_CSTR_DUP_OPLIST, RHSRecordData, M_POD_OPLIST)

typedef struct {
    RHSMutex* mutex;
    RHSRecordDataDict_t records;
} RHSRecord;

static RHSRecord* rhs_record = NULL;

static RHSRecordData* rhs_record_get(const char* name) {
    return RHSRecordDataDict_get(rhs_record->records, name);
}

static void rhs_record_put(const char* name, RHSRecordData* record_data) {
    RHSRecordDataDict_set_at(rhs_record->records, name, *record_data);
}

static void rhs_record_erase(const char* name, RHSRecordData* record_data) {
    rhs_event_flag_free(record_data->flags);
    RHSRecordDataDict_erase(rhs_record->records, name);
}

void rhs_record_init(void) {
    rhs_record = malloc(sizeof(RHSRecord));
    rhs_record->mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
    RHSRecordDataDict_init(rhs_record->records);
}

static RHSRecordData* rhs_record_data_get_or_create(const char* name) {
    rhs_assert(rhs_record);
    RHSRecordData* record_data = rhs_record_get(name);
    if(!record_data) {
        RHSRecordData new_record;
        new_record.flags = rhs_event_flag_alloc();
        new_record.data = NULL;
        new_record.holders_count = 0;
        rhs_record_put(name, &new_record);
        record_data = rhs_record_get(name);
    }
    return record_data;
}

static void rhs_record_lock(void) {
    rhs_assert(rhs_mutex_acquire(rhs_record->mutex, RHSWaitForever) == RHSStatusOk);
}

static void rhs_record_unlock(void) {
    rhs_assert(rhs_mutex_release(rhs_record->mutex) == RHSStatusOk);
}

bool rhs_record_exists(const char* name) {
    rhs_assert(rhs_record);
    rhs_assert(name);

    bool ret = false;

    rhs_record_lock();
    ret = (rhs_record_get(name) != NULL);
    rhs_record_unlock();

    return ret;
}

void rhs_record_create(const char* name, void* data) {
    rhs_assert(rhs_record);
    rhs_assert(name);
    rhs_assert(data);

    rhs_record_lock();

    // Get record data and fill it
    RHSRecordData* record_data = rhs_record_data_get_or_create(name);
    rhs_assert(record_data->data == NULL);
    record_data->data = data;
    rhs_event_flag_set(record_data->flags, RHS_RECORD_FLAG_READY);

    rhs_record_unlock();
}

bool rhs_record_destroy(const char* name) {
    rhs_assert(rhs_record);
    rhs_assert(name);

    bool ret = false;

    rhs_record_lock();

    RHSRecordData* record_data = rhs_record_get(name);
    rhs_assert(record_data);
    if(record_data->holders_count == 0) {
        rhs_record_erase(name, record_data);
        ret = true;
    }

    rhs_record_unlock();

    return ret;
}

void* rhs_record_open(const char* name) {
    rhs_assert(rhs_record);
    rhs_assert(name);

    rhs_record_lock();

    RHSRecordData* record_data = rhs_record_data_get_or_create(name);
    record_data->holders_count++;

    rhs_record_unlock();

    // Wait for record to become ready
    rhs_assert(
        rhs_event_flag_wait(
            record_data->flags,
            RHS_RECORD_FLAG_READY,
            RHSFlagWaitAny | RHSFlagNoClear,
            RHSWaitForever) == RHS_RECORD_FLAG_READY);

    return record_data->data;
}

void rhs_record_close(const char* name) {
    rhs_assert(rhs_record);
    rhs_assert(name);

    rhs_record_lock();

    RHSRecordData* record_data = rhs_record_get(name);
    rhs_assert(record_data);
    record_data->holders_count--;

    rhs_record_unlock();
}
