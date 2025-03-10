#include <stdint.h>

typedef struct {
    // Time
    uint32_t hours; /**< Hour in 24H format: 0-23 */
    uint32_t minutes; /**< Minute: 0-59 */
    uint32_t seconds; /**< Second: 0-59 */
    uint32_t mseconds;
    // Date
    uint32_t day; /**< Current day: 1-31 */
    uint32_t month; /**< Current month: 1-12 */
    uint32_t year; /**< Current year: 2000-2099 */
    uint32_t weekday; /**< Current weekday: 1-7 */
} datetime_t;

datetime_t convert_unix_time_2_date(uint64_t seconds);

uint32_t convert_date_2_unix_time(const datetime_t* date);
