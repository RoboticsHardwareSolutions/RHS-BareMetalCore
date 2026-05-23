#pragma once

#include "stdint.h"
#include "stdio.h"

/**
 * @brief Parse IP address string into four octets
 * @param ip_str Input string in format "192.168.1.100"
 * @param a Pointer to first octet
 * @param b Pointer to second octet
 * @param c Pointer to third octet
 * @param d Pointer to fourth octet
 * @return 0 on success, -1 on error
 */
int string_to_ip(const char* ip_str, unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);

/**
 * @brief Convert four octets into IP address string
 * @param a First octet
 * @param b Second octet
 * @param c Third octet
 * @param d Fourth octet
 * @param str_buf Buffer to store resulting string (must be at least 16 bytes)
 * @return 0 on success, -1 on error
 */
int ip_to_string(unsigned int a, unsigned int b, unsigned int c, unsigned int d, char* str_buf);

/**
 * @brief Check if string is a valid IPv4 address
 * @param ip_str Input string to validate
 * @return 1 if valid, 0 if invalid
 */
int ip_string_is_valid(const char* ip_str);
