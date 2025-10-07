#include "stdint.h"
#include "stdio.h"

/**
 * @brief Parse single octet from string
 * @param str Pointer to string pointer (will be updated)
 * @param value Pointer to store parsed value
 * @return 0 on success, -1 on error
 */
static int parse_octet(const char** str, unsigned int* value)
{
    const char* p   = *str;
    *value          = 0;
    int digit_count = 0;

    while (*p >= '0' && *p <= '9')
    {
        *value = *value * 10 + (*p - '0');
        p++;
        digit_count++;
        if (*value > 255)
            return -1;
    }

    // At least one digit must be present
    if (digit_count == 0)
        return -1;

    *str = p;
    return 0;
}

/**
 * @brief Parse IP address string into four octets
 * @param ip_str Input string in format "192.168.1.100"
 * @param a Pointer to first octet
 * @param b Pointer to second octet
 * @param c Pointer to third octet
 * @param d Pointer to fourth octet
 * @return 0 on success, -1 on error
 */
int string_to_ip(const char* ip_str, unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
{
    if (ip_str == NULL || a == NULL || b == NULL || c == NULL || d == NULL)
    {
        return -1;
    }

    // Skip leading whitespace
    while (*ip_str == ' ' || *ip_str == '\t')
        ip_str++;

    // Parse four octets separated by dots
    if (parse_octet(&ip_str, a) != 0)
        return -1;
    if (*ip_str++ != '.')
        return -1;

    if (parse_octet(&ip_str, b) != 0)
        return -1;
    if (*ip_str++ != '.')
        return -1;

    if (parse_octet(&ip_str, c) != 0)
        return -1;
    if (*ip_str++ != '.')
        return -1;

    if (parse_octet(&ip_str, d) != 0)
        return -1;

    // Check that we reached end of string or whitespace
    if (*ip_str != '\0' && *ip_str != ' ' && *ip_str != '\t' && *ip_str != '\r' && *ip_str != '\n')
        return -1;

    return 0;
}

// static int string_to_ip(const char* str_ip, uint32_t* ip_addr)
// {
//     if (str_ip == NULL || ip_addr == NULL)
//     {
//         return -1;
//     }

//     unsigned int octet1, octet2, octet3, octet4;
//     if (parse_ip_address(str_ip, &octet1, &octet2, &octet3, &octet4) != 0)
//     {
//         return -1;
//     }

//     // Validate octets
//     if (octet1 > 255 || octet2 > 255 || octet3 > 255 || octet4 > 255)
//     {
//         return -1;
//     }

//     // Convert to network byte order
//     uint8_t* octets = (uint8_t*) ip_addr;
//     octets[3]       = (uint8_t) octet1;
//     octets[2]       = (uint8_t) octet2;
//     octets[1]       = (uint8_t) octet3;
//     octets[0]       = (uint8_t) octet4;

//     return 0;
// }
int ip_to_string(unsigned int a, unsigned int b, unsigned int c, unsigned int d, char* str_buf)
{
    if (str_buf == NULL)
    {
        return -1;
    }

    // Validate octets
    if (a > 255 || b > 255 || c > 255 || d > 255)
    {
        return -1;
    }

    sprintf(str_buf, "%u.%u.%u.%u", a, b, c, d);

    return 0;
}
