#include "unixtime.h"

datetime_t convert_unix_time_2_date(uint64_t seconds)
{
    datetime_t           date_time;
    static unsigned char month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static unsigned char week_days[7]   = {4, 5, 6, 0, 1, 2, 3};

    unsigned char leap_days, leap_year_ind;

    unsigned short temp_days;

    uint64_t     epoch = seconds;
    unsigned int days_since_epoch, day_of_year;

    leap_days     = 0;
    leap_year_ind = 0;

    /* Add or substract time zone here. */
    epoch += 0; /* GMT +0:00 = +0 seconds */

    date_time.seconds = epoch % 60;
    epoch /= 60;
    date_time.minutes = epoch % 60;
    epoch /= 60;
    date_time.hours = epoch % 24;
    epoch /= 24;

    days_since_epoch = epoch;                           /* number of days since epoch */
    date_time.weekday     = week_days[days_since_epoch % 7]; /* Calculating WeekDay */

    date_time.year = 1970 + (days_since_epoch / 365); /* ball parking year, may not be accurate! */

    for (int i = 1972; i < date_time.year; i += 4) /* Calculating number of leap days since epoch/1970 */
    {
        if (((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0))
        {
            leap_days++;
        }
    }

    /* Calculating accurate current year by (days_since_epoch - extra leap days) */
    date_time.year    = 1970 + ((days_since_epoch - leap_days) / 365);
    day_of_year = ((days_since_epoch - leap_days) % 365) + 1;

    if (((date_time.year % 4 == 0) && (date_time.year % 100 != 0)) || (date_time.year % 400 == 0))
    {
        month_days[1] = 29; /* February = 29 days for leap years */
        leap_year_ind = 1;  /* if current year is leap, set indicator to 1 */
    }
    else
    {
        month_days[1] = 28; /* February = 28 days for non-leap years */
    }

    temp_days = 0;

    for (date_time.month = 0; date_time.month <= 11; date_time.month++) /* calculating current Month */
    {
        if (day_of_year <= temp_days)
            break;
        temp_days = temp_days + month_days[date_time.month];
    }

    temp_days = temp_days - month_days[date_time.month - 1]; /* calculating current Date */
    date_time.day  = day_of_year - temp_days;
    return date_time;
}


uint32_t convert_date_2_unix_time(const datetime_t*date)
{
    uint32_t y;
    uint32_t m;
    uint32_t d;
    uint32_t t;

    y = date->year; /* Year */
    m = date->month; /* Month of year */
    d = date->day; /* Day of month */

    //January and February are counted as months 13 and 14 of the previous year */
    if(m <= 2)
    {
        m += 12;
        y -= 1;
    }

    /* Convert years to days */
    t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
    /* Convert months to days */
    t += (30 * m) + (3 * (m + 1) / 5) + d;
    /* Unix time starts on January 1st, 1970 */
    t -= 719561;
    /* Convert days to seconds */
    t *= 86400;
    /* Add hours, minutes and seconds */
    t += (3600 * date->hours) + (60 * date->minutes) + date->seconds;

    return t; /* Return Unix time */
}
