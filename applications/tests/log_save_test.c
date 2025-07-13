#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "rhs.h"
#include "rhs_hal.h"
#include "cli.h"
#include "runit.h"

#define TAG "log_save_test"

void log_save_test(char* args, void* context)
{
    extern uint32_t max_log_count;
    extern uint32_t max_log_length;
    runit_counter_assert_passes = 0;
    runit_counter_assert_failures = 0;


    /* One saved message can had no more 120 bytes */
    rhs_erase_saved_log();
    runit_assert(rhs_count_saved_log() == 0);

    /* Save the first log message */
    rhs_log_save("Test message 1");
    runit_assert(rhs_count_saved_log() == 1);
    char* log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, "Test message 1") == 0);

    /* Save multiple messages */
    for (int i = 2; i <= max_log_count; i++)
    {
        char message[32];
        snprintf(message, sizeof(message), "Test message %d", i);
        rhs_log_save("%s", message);
    }
    runit_assert(rhs_count_saved_log() == max_log_count);
    for (int i = 0; i < max_log_count; i++)
    {
        char expected_message[32];
        snprintf(expected_message, sizeof(expected_message), "Test message %d", i + 1);
        char* log_message = rhs_read_saved_log(i);
        runit_assert(strcmp(log_message, expected_message) == 0);
    }
    rhs_erase_saved_log();
    runit_assert(rhs_count_saved_log() == 0);

#define OVERFLOW 16
    /* Test: Save more than max count, ensure only last 16 are kept and old logs are gone */
    rhs_erase_saved_log();
    for (int i = 1; i <= max_log_count + OVERFLOW; i++)
    {
        char message[32];
        snprintf(message, sizeof(message), "Overflow message %d", i);
        rhs_log_save("%s", message);
    }
    runit_assert(rhs_count_saved_log() == OVERFLOW);
    // Only messages 17 to 32 should be present
    for (int i = 0; i < OVERFLOW; i++)
    {
        char expected_message[32];
        snprintf(expected_message, sizeof(expected_message), "Overflow message %d", max_log_count + i + 1);
        char* log_message = rhs_read_saved_log(i);
        runit_assert(strcmp(log_message, expected_message) == 0);
    }
    rhs_erase_saved_log();
    runit_assert(rhs_count_saved_log() == 0);

    /* Test saving a string exactly at the max allowed length (120 bytes) */
    rhs_erase_saved_log();
    char max_length_string[max_log_length];
    memset(max_length_string, 'B', sizeof(max_length_string) - 1);
    max_length_string[max_log_length - 1] = '\0';  // 120 characters + null terminator
    rhs_log_save("%s", max_length_string);
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, max_length_string) == 0);
    rhs_erase_saved_log();
    runit_assert(rhs_count_saved_log() == 0);

    /* Test saving a string just over the max allowed length (121 bytes) */
    rhs_erase_saved_log();
    char over_max_length_string[max_log_length + 1];
    memset(over_max_length_string, 'C', sizeof(over_max_length_string) - 1);
    over_max_length_string[max_log_length] = '\0';  // 121 characters + null terminator
    rhs_log_save("%s", over_max_length_string);
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(log_message[max_log_length - 1] == '\0');
    rhs_erase_saved_log();
    runit_assert(rhs_count_saved_log() == 0);

    /* Test saving an empty string */
    rhs_erase_saved_log();
    rhs_log_save("");
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, "") == 0);
    rhs_erase_saved_log();
    runit_assert(rhs_count_saved_log() == 0);

    /* Test: Save a log message with template formatting (integer) */
    rhs_erase_saved_log();
    int value = 42;
    rhs_log_save("Template integer: %d", value);
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, "Template integer: 42") == 0);
    rhs_erase_saved_log();

    /* Test: Save a log message with template formatting (string) */
    const char* str_val = "abc";
    rhs_log_save("Template string: %s", str_val);
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, "Template string: abc") == 0);
    rhs_erase_saved_log();

    /* Test: Save a log message with multiple template arguments */
    int a = 7, b = 13;
    rhs_log_save("Sum of %d and %d is %d", a, b, a + b);
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, "Sum of 7 and 13 is 20") == 0);
    rhs_erase_saved_log();

    /* Test: Save a log message with special characters */
    rhs_log_save("Special chars: !@#$%%^&*()_+-=[]{}|;':,.<>/?");
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, "Special chars: !@#$%^&*()_+-=[]{}|;':,.<>/?") == 0);
    rhs_erase_saved_log();

    /* Test: Save a log message with percent sign */
    rhs_log_save("Percent: 100%% complete");
    runit_assert(rhs_count_saved_log() == 1);
    log_message = rhs_read_saved_log(0);
    runit_assert(strcmp(log_message, "Percent: 100% complete") == 0);
    rhs_erase_saved_log();

    runit_report();
}

void log_save_test_start_up(void)
{
    Cli* cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "log_save_test", log_save_test, NULL);
    rhs_record_close(RECORD_CLI);
}
