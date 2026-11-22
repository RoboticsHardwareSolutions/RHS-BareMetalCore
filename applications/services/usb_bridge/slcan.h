#pragma once

/* SLCAN (Lawicel CAN232/CANUSB ASCII protocol) parser and formatter, classic CAN only.
 * Plain C without RHS dependencies: only the RHSHalCANFrameType type from rhs_hal_can.h. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "rhs_hal_can.h"

/** Longest command line without CR: "T" + 8 id + 1 dlc + 16 data */
#define SLCAN_LINE_MAX 26

/** Longest formatted frame: "T" + 8 id + 1 dlc + 16 data + CR */
#define SLCAN_FRAME_MAX (SLCAN_LINE_MAX + 1)

#define SLCAN_OK "\r"
#define SLCAN_ERROR "\a"

typedef enum
{
    SlcanCmdNone,        /**< Empty line */
    SlcanCmdInvalid,     /**< Unknown command or malformed arguments: answer SLCAN_ERROR */
    SlcanCmdSetBitrate,  /**< Sn: bitrate */
    SlcanCmdOpen,        /**< O */
    SlcanCmdListenOnly,  /**< L */
    SlcanCmdClose,       /**< C */
    SlcanCmdTransmit,    /**< t, T, r, R: frame */
    SlcanCmdStatus,      /**< F */
    SlcanCmdVersion,     /**< V */
    SlcanCmdSwVersion,   /**< v */
    SlcanCmdSerial,      /**< N */
    SlcanCmdTimestamp,   /**< Zn: timestamp */
    SlcanCmdAcceptFilter /**< Mxxxxxxxx / mxxxxxxxx: accepted and ignored, all frames pass */
} SlcanCmdType;

typedef struct
{
    SlcanCmdType       type;
    uint32_t           bitrate;   /**< SlcanCmdSetBitrate */
    bool               timestamp; /**< SlcanCmdTimestamp */
    RHSHalCANFrameType frame;     /**< SlcanCmdTransmit */
} SlcanCmd;

/** Splits the byte stream from the host into lines */
typedef struct
{
    char   buf[SLCAN_LINE_MAX];
    size_t len;
    bool   overflow;
} SlcanReader;

void slcan_reader_reset(SlcanReader* reader);

/** Feed one byte.
 * @return true when a line is complete (CR received): @p cmd is filled. LF is ignored, a line longer than
 *         SLCAN_LINE_MAX gives SlcanCmdInvalid. */
bool slcan_reader_push(SlcanReader* reader, char c, SlcanCmd* cmd);

/** Parse one line without the terminating CR */
void slcan_parse(const char* line, size_t len, SlcanCmd* cmd);

/** Sn index (0..8) to bitrate, 0 for an unknown index */
uint32_t slcan_bitrate_from_index(uint8_t index);

/** Format @p frame as t/T/r/R with the terminating CR.
 * @param out buffer of at least SLCAN_FRAME_MAX bytes, no terminating NUL is written
 * @return number of bytes written, 0 when the frame cannot be represented */
size_t slcan_format_frame(const RHSHalCANFrameType* frame, char* out);
