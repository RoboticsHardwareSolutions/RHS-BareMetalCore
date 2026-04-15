/**
 * @file event_flag.h
 * RHS Event Flag
 */
#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RHSEventFlag RHSEventFlag;

/** Allocate RHSEventFlag
 *
 * @return     pointer to RHSEventFlag
 */
RHSEventFlag* rhs_event_flag_alloc(void);

/** Deallocate RHSEventFlag
 *
 * @param      instance  pointer to RHSEventFlag
 */
void rhs_event_flag_free(RHSEventFlag* instance);

/** Set flags
 *
 * @warning    result of this function can be flags that you've just asked to
 *             set or not if someone was waiting for them and asked to clear it.
 *             It is highly recommended to read this function and
 *             xEventGroupSetBits source code.
 *
 * @param      instance  pointer to RHSEventFlag
 * @param[in]  flags     The flags to set
 *
 * @return     Resulting flags(see warning) or error (RHSStatus)
 */
uint32_t rhs_event_flag_set(RHSEventFlag* instance, uint32_t flags);

/** Clear flags
 *
 * @param      instance  pointer to RHSEventFlag
 * @param[in]  flags     The flags
 *
 * @return     Resulting flags or error (RHSStatus)
 */
uint32_t rhs_event_flag_clear(RHSEventFlag* instance, uint32_t flags);

/** Get flags
 *
 * @param      instance  pointer to RHSEventFlag
 *
 * @return     Resulting flags
 */
uint32_t rhs_event_flag_get(RHSEventFlag* instance);

/**
* @brief Waits for specified event flags to be set with optional timeout
*
* This function provides a fundamental synchronization primitive for multi-threaded
* real-time embedded systems. It blocks the calling thread until one or more specified
* event flags are set, or until a timeout occurs. The function is designed to work
* seamlessly with RTOS schedulers and interrupt service routines.
*
* CORE FUNCTIONALITY:
* - Thread-safe waiting for multiple event conditions
* - Configurable wait behavior (any flag vs all flags)
* - Optional automatic flag clearing after successful wait
* - Timeout support ranging from non-blocking to infinite wait
* - Integration with RTOS scheduling and power management
*
* SYNCHRONIZATION PATTERNS:
*
* 1. PRODUCER-CONSUMER COORDINATION:
*    Used extensively for coordinating data producers (ISRs, timers) with consumers
*    (application threads). Ensures data consistency and prevents race conditions.
*
* 2. SERVICE INITIALIZATION BARRIERS:
*    Multiple threads can wait for service readiness before proceeding with their
*    initialization sequences. Critical for dependency management in complex systems.
*
* 3. REAL-TIME EVENT PROCESSING:
*    Enables responsive handling of time-critical events while maintaining
*    deterministic behavior and meeting real-time constraints.
*
* 4. RESOURCE AVAILABILITY SIGNALING:
*    Coordinates access to shared resources, hardware peripherals, and communication
*    channels in a thread-safe manner.
*
* THREAD SAFETY AND ATOMICITY:
* - All flag operations are atomic with respect to other threads
* - No race conditions between flag setting and waiting operations
* - Safe for use in multi-core environments with proper memory barriers
* - Compatible with priority inheritance and priority ceiling protocols
*
* REAL-TIME CONSIDERATIONS:
* - Deterministic execution time for most common use cases
* - Minimal interrupt latency impact when called from high-priority contexts
* - Supports priority-based scheduling decisions during blocking operations
* - Memory usage is bounded and predictable for real-time memory allocators
*
* @param      instance  Pointer to the RHSEventFlag object to wait on.
*                      Must be a valid instance previously allocated with rhs_event_flag_alloc().
*                      The instance maintains internal state including:
*                      - Current flag values (32-bit mask)
*                      - List of waiting threads with their conditions
*                      - RTOS synchronization primitives
*                      - Optional timeout management structures
*
*                      THREAD SAFETY: Multiple threads can safely wait on the same
*                      instance simultaneously with different flag conditions.
*
* @param[in]  flags     Bitmask specifying which flags to wait for (32-bit value).
*
*                      IMPORTANT: Not all 32 bits may be available for user applications.
*                      The actual number of usable flag bits depends on system implementation
*                      and reserved flags used internally by the RTOS and system services.
*
*                      TYPICAL AVAILABILITY:
*                      - User flags: Usually bits 0-23 (24 flags) or 0-15 (16 flags)
*                      - System reserved: Upper bits may be reserved for internal use
*                      - Check system documentation for exact bit allocation
*
*                      USABLE BIT POSITIONS (example for 24-bit user space):
*                      - Bit 0 (0x00000001): User event condition 0
*                      - Bit 1 (0x00000002): User event condition 1
*                      - ...
*                      - Bit 23 (0x00800000): User event condition 23
*                      - Bits 24-31: Reserved for system use - DO NOT USE
*
*                      COMMON PATTERNS (within available bit range):
*                      - Single flag: 0x00000001, 0x00000002, 0x00000004, etc.
*                      - Multiple flags: 0x00000003 (flags 0 and 1), 0x0000000F (flags 0-3)
*                      - User flags mask: 0x00FFFFFF (for 24-bit user space)
*
*                      DESIGN GUIDELINES:
*                      - Use meaningful bit positions for different event types within user range
*                      - Reserve lower bits (0-3) for "ready" or "initialized" states
*                      - Group related flags in adjacent bit positions
*                      - Document flag meanings clearly in system design
*                      - NEVER use reserved high-order bits
*                      - Define flag constants to avoid magic numbers:
*                        #define SERVICE_READY_FLAG    (0x00000001)
*                        #define DATA_AVAILABLE_FLAG   (0x00000002)
*                        #define ERROR_OCCURRED_FLAG   (0x00000004)

*
* @param[in]  options   Bitfield controlling wait behavior and flag management.
*                      Multiple options can be combined using bitwise OR (|).
*
*                      KEY OPTIONS:
*
*                      RHSFlagWaitAny (typically 0x00):
*                      - Wait until ANY of the specified flags is set
*                      - Most common usage pattern (>95% of cases)
*                      - Enables responsive event handling
*                      - Returns immediately when first matching flag is detected
*
*                      RHSFlagWaitAll (typically 0x01):
*                      - Wait until ALL specified flags are set simultaneously
*                      - Used for complex synchronization scenarios
*                      - Higher risk of indefinite blocking if conditions aren't met
*                      - Useful for multi-stage initialization sequences
*
*                      RHSFlagNoClear (typically 0x02):
*                      - Prevent automatic clearing of flags after successful wait
*                      - CRITICAL for shared resources accessed by multiple threads
*                      - Allows "broadcast" semantics where one event notifies many waiters
*                      - Must be explicitly cleared using rhs_event_flag_clear()
*
*                      COMBINATION EXAMPLES:
*                      - (RHSFlagWaitAny): Wait for any flag, clear after match
*                      - (RHSFlagWaitAny | RHSFlagNoClear): Wait for any flag, don't clear
*                      - (RHSFlagWaitAll | RHSFlagNoClear): Wait for all flags, don't clear
*
* @param[in]  timeout   Maximum time to wait in milliseconds (platform-dependent units).
*                      Controls blocking behavior and system responsiveness:
*
*                      SPECIAL VALUES:
*
*                      RHSWaitForever (typically 0xFFFFFFFF):
*                      - Block indefinitely until flags are set
*                      - Use ONLY when flag setting is guaranteed by system design
*                      - Risk: Can cause system deadlock if flags never set
*                      - Benefits: Simplifies error handling, lowest power consumption
*                      - Typical use: Service initialization, resource availability
*
*                      0 (zero):
*                      - Non-blocking check (polling mode)
*                      - Returns immediately with current flag state
*                      - Safe for ISR context (with platform limitations)
*                      - Use for: State queries, conditional processing
*                      - Higher CPU usage if called repeatedly
*
*                      FINITE TIMEOUTS (1 to 0xFFFFFFFE):
*                      - Balance between responsiveness and reliability
*                      - Enable timeout-based error recovery
*                      - Typical ranges:
*                        * 1-10ms: Real-time event processing
*                        * 100-1000ms: Network operations, device communication
*                        * 5000-30000ms: System initialization, complex operations
*
*                      TIMEOUT RESOLUTION:
*                      - Actual resolution depends on RTOS tick rate
*                      - Common tick rates: 1ms (high precision), 10ms (balanced), 100ms (low power)
*                      - Timeout accuracy: ±1 tick in most implementations
*                      - Consider tick granularity when setting short timeouts
*
* @return     Function returns different types of values based on execution outcome:
*
*            SUCCESS CASES (Flags were set):
*            Returns the bitmask of flags that were actually set at the time of return.
*            This value will always have a non-zero intersection with the requested flags.
*
*            IMPORTANT: The returned value may include additional flags beyond those
*            requested, as other system components may have set different flags
*            concurrently. Always mask the result with your requested flags to check
*            for specific conditions:
*
*            Example:
*            uint32_t result = rhs_event_flag_wait(flag, 0x07, RHSFlagWaitAny, 1000);
 *           if (result == RHSStatusErrorTimeout){
 *              return ;
 *           }
*            if (result & 0x01) { handle_event_1(); }
*            if (result & 0x02) { handle_event_2(); }
*            if (result & 0x04) { handle_event_3(); }
*
*            ERROR CASES (encoded as specific uint32_t values):
*
*            RHSStatusErrorTimeout (typically 0xFFFFFFFE):
*            - Timeout expired before any requested flags were set
*            - Most common error condition in well-designed systems
*            - Indicates need for retry logic or alternative processing path
*            - NOT necessarily a system error - may be expected behavior
*
*            RHSStatusErrorParameter (typically 0xFFFFFFFD):
*            - Invalid function parameters detected:
*              * NULL instance pointer
*              * flags == 0 (no flags specified)
*              * Invalid options combination
*              * Reserved option bits set
*            - Programming error - should be fixed during development
*            - May indicate memory corruption or uninitialized variables
*
*            RHSStatusErrorISR (typically 0xFFFFFFFC):
*            - Function called from interrupt context with blocking timeout
*            - Blocking operations are prohibited in ISRs on most platforms
*            - Use timeout=0 for ISR-safe polling, or defer to thread context
*            - Design error - restructure code to avoid blocking in ISRs
*
*            RHSStatusError (typically 0xFFFFFFFF):
*            - System-level error occurred:
*              * Memory allocation failure
*              * RTOS scheduling error
*              * Hardware fault detected
*              * Resource exhaustion
*            - May indicate serious system problems requiring recovery
*            - Consider system reset or safe mode transition
*
* @warning   ISR CONTEXT RESTRICTIONS:
*           - NEVER call with timeout != 0 from interrupt service routines
*           - ISR violations can cause system crashes, priority inversions, or deadlocks
*           - Use timeout=0 for ISR-safe flag checking
*           - Consider using rhs_event_flag_set_isr() from ISRs instead
*
* @warning   DEADLOCK PREVENTION:
*           - When using RHSWaitForever, ensure flag setting is guaranteed
*           - Avoid circular dependencies between event flags
*           - Implement watchdog monitoring for critical threads
*           - Consider timeout-based fallback strategies
*
* @warning   MEMORY ORDERING:
*           - Flag operations include appropriate memory barriers
*           - Ensures visibility of flag changes across CPU cores
*           - Data accessed after successful flag wait is guaranteed to be current
*           - No additional synchronization needed for flag-protected data
*
* @note      PLATFORM DEPENDENCIES:
*           - Built on platform-specific RTOS primitives (FreeRTOS, ThreadX, etc.)
*           - Timeout units and precision vary by platform configuration
*           - Maximum supported timeout values are platform-dependent
*           - Some platforms may have restrictions on concurrent waiters
*
* @note      POWER MANAGEMENT:
*           - Blocked threads consume minimal CPU resources
*           - May enable low-power sleep modes on supporting platforms
*           - Wake-up latency depends on power management settings
*           - Consider power implications for battery-operated systems
*
* @note      PERFORMANCE CHARACTERISTICS:
*           - Non-blocking calls (timeout=0): O(1) execution time
*           - Flag setting operations: O(n) where n = number of waiting threads
*           - Memory usage: Fixed per instance + variable per waiting thread
*           - Context switch overhead only occurs during blocking/unblocking
*
* @example   COMMON USAGE PATTERNS:
*
*           // 1. Service initialization barrier
*           uint32_t result = rhs_event_flag_wait(service_flags,
*                                                SERVICE_READY_FLAG,
*                                                RHSFlagWaitAny | RHSFlagNoClear,
*                                                RHSWaitForever);
*          if (events == RHSStatusErrorTimeout) {
*               // Timeout occurred, implement fallback behavior
*               use_cached_sensor_data();
*           }
*           else if (result == SERVICE_READY_FLAG) {
*               // Service is ready, proceed with initialization
*           }
*
*           // 2. Real-time event processing with timeout
*           uint32_t events = rhs_event_flag_wait(sensor_flags,
*                                                SENSOR_DATA_READY | SENSOR_ERROR,
*                                                RHSFlagWaitAny,
*                                                100); // 100ms timeout
*           if (events & SENSOR_DATA_READY) {
*               process_sensor_data();
*           } else if (events & SENSOR_ERROR) {
*               handle_sensor_error();
*           } else
*
*           // 3. Non-blocking status check
*           uint32_t status = rhs_event_flag_wait(status_flags,
*                                                OPERATION_COMPLETE,
*                                                RHSFlagWaitAny,
*                                                0); // No blocking
*           if (status & OPERATION_COMPLETE) {
*               // Operation completed
*           } else {
*               // Still in progress, continue other work
*           }
*
*           // 4. Multi-condition synchronization
*           uint32_t ready = rhs_event_flag_wait(init_flags,
*                                               HARDWARE_INIT | SOFTWARE_INIT | CONFIG_LOADED,
*                                               RHSFlagWaitAll,
*                                               30000); // 30 second timeout
*           if (ready == (HARDWARE_INIT | SOFTWARE_INIT | CONFIG_LOADED)) {
*               // All subsystems initialized, start main application
*               start_main_application();
*           } else {
*               // Initialization failed or timed out
*               enter_safe_mode();
*           }
*/
uint32_t rhs_event_flag_wait(RHSEventFlag* instance, uint32_t flags, uint32_t options, uint32_t timeout);

#ifdef __cplusplus
}
#endif