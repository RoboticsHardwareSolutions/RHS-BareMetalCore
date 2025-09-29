#include "rhs.h"
#include "runit.h"
#include "cli.h"

#define TEST_RECORD_NAME "test_record"

void records_test(char* args, void* context)
{
    runit_counter_assert_passes   = 0;
    runit_counter_assert_failures = 0;

    // Test that record does not exist
    runit_assert(rhs_record_exists(TEST_RECORD_NAME) == false);

    // Create record
    uint8_t test_data = 0;
    rhs_record_create(TEST_RECORD_NAME, (void*)&test_data);

    // Test that record exists
    runit_assert(rhs_record_exists(TEST_RECORD_NAME) == true);

    // Open it
    void* record = rhs_record_open(TEST_RECORD_NAME);
    runit_eq(record, &test_data);

    // Close it
    rhs_record_close(TEST_RECORD_NAME);

    // Clean up
    rhs_record_destroy(TEST_RECORD_NAME);

    // Test that record does not exist
    runit_assert(rhs_record_exists(TEST_RECORD_NAME) == false);

    runit_report();
}

void rhs_records_test(void)
{
    Cli *cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "records_test", records_test, NULL);
    rhs_record_close(RECORD_CLI);
}