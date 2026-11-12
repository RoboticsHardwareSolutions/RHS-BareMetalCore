#include "slcan.h"

#define SLCAN_STD_ID_MAX 0x7FFU
#define SLCAN_EXT_ID_MAX 0x1FFFFFFFU

static const char hex_digits[] = "0123456789ABCDEF";

static const uint32_t bitrates[] = {10000, 20000, 50000, 100000, 125000, 250000, 500000, 800000, 1000000};

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

static bool parse_hex(const char* s, size_t digits, uint32_t* value)
{
    uint32_t v = 0;
    for (size_t i = 0; i < digits; i++)
    {
        int d = hex_value(s[i]);
        if (d < 0)
            return false;
        v = (v << 4) | (uint32_t) d;
    }
    *value = v;
    return true;
}

static void put_hex(char* out, uint32_t value, size_t digits)
{
    for (size_t i = 0; i < digits; i++)
    {
        out[digits - 1 - i] = hex_digits[value & 0xF];
        value >>= 4;
    }
}

uint32_t slcan_bitrate_from_index(uint8_t index)
{
    if (index >= sizeof(bitrates) / sizeof(bitrates[0]))
        return 0;
    return bitrates[index];
}

/* tiiildd.. / Tiiiiiiiildd.. / riiil / Riiiiiiiil */
static bool parse_frame(const char* line, size_t len, RHSHalCANFrameType* frame)
{
    bool     ext       = (line[0] == 'T' || line[0] == 'R');
    bool     rtr       = (line[0] == 'r' || line[0] == 'R');
    size_t   id_digits = ext ? 8 : 3;
    uint32_t id;
    uint32_t dlc;

    if (len < 1 + id_digits + 1)
        return false;
    if (!parse_hex(&line[1], id_digits, &id) || id > (ext ? SLCAN_EXT_ID_MAX : SLCAN_STD_ID_MAX))
        return false;
    if (!parse_hex(&line[1 + id_digits], 1, &dlc) || dlc > 8)
        return false;

    size_t data_pos = 1 + id_digits + 1;
    if (len != data_pos + (rtr ? 0 : dlc * 2))
        return false;

    frame->id   = id;
    frame->len  = (uint8_t) dlc;
    frame->type = ext ? FrameTypeExtID : FrameTypeStdID;
    frame->rtr  = rtr;
    for (size_t i = 0; i < sizeof(frame->payload); i++)
    {
        uint32_t byte = 0;
        if (!rtr && i < dlc && !parse_hex(&line[data_pos + i * 2], 2, &byte))
            return false;
        frame->payload[i] = (uint8_t) byte;
    }
    return true;
}

void slcan_parse(const char* line, size_t len, SlcanCmd* cmd)
{
    cmd->type = SlcanCmdInvalid;

    if (len == 0)
    {
        cmd->type = SlcanCmdNone;
        return;
    }

    switch (line[0])
    {
    case 'S':
        if (len == 2 && line[1] >= '0' && line[1] <= '9')
        {
            cmd->bitrate = slcan_bitrate_from_index((uint8_t) (line[1] - '0'));
            if (cmd->bitrate != 0)
                cmd->type = SlcanCmdSetBitrate;
        }
        break;
    case 'O':
    case 'L':
    case 'C':
    case 'F':
    case 'V':
    case 'v':
    case 'N':
        if (len == 1)
        {
            static const struct
            {
                char         c;
                SlcanCmdType type;
            } simple[] = {
                {'O', SlcanCmdOpen},
                {'L', SlcanCmdListenOnly},
                {'C', SlcanCmdClose},
                {'F', SlcanCmdStatus},
                {'V', SlcanCmdVersion},
                {'v', SlcanCmdSwVersion},
                {'N', SlcanCmdSerial},
            };
            for (size_t i = 0; i < sizeof(simple) / sizeof(simple[0]); i++)
            {
                if (simple[i].c == line[0])
                    cmd->type = simple[i].type;
            }
        }
        break;
    case 'Z':
        if (len == 2 && (line[1] == '0' || line[1] == '1'))
        {
            cmd->timestamp = (line[1] == '1');
            cmd->type      = SlcanCmdTimestamp;
        }
        break;
    case 'M':
    case 'm':
    {
        uint32_t unused;
        if (len == 9 && parse_hex(&line[1], 8, &unused))
            cmd->type = SlcanCmdAcceptFilter;
        break;
    }
    case 't':
    case 'T':
    case 'r':
    case 'R':
        if (parse_frame(line, len, &cmd->frame))
            cmd->type = SlcanCmdTransmit;
        break;
    default:
        break;
    }
}

void slcan_reader_reset(SlcanReader* reader)
{
    reader->len      = 0;
    reader->overflow = false;
}

bool slcan_reader_push(SlcanReader* reader, char c, SlcanCmd* cmd)
{
    if (c == '\n')
        return false;

    if (c == '\r')
    {
        if (reader->overflow)
            cmd->type = SlcanCmdInvalid;
        else
            slcan_parse(reader->buf, reader->len, cmd);
        slcan_reader_reset(reader);
        return true;
    }

    if (reader->len < sizeof(reader->buf))
        reader->buf[reader->len++] = c;
    else
        reader->overflow = true;
    return false;
}

size_t slcan_format_frame(const RHSHalCANFrameType* frame, char* out)
{
    bool ext = (frame->type == FrameTypeExtID);

    if (frame->type != FrameTypeStdID && !ext)
        return 0;
    if (frame->id > (ext ? SLCAN_EXT_ID_MAX : SLCAN_STD_ID_MAX) || frame->len > 8)
        return 0;

    size_t pos = 0;
    out[pos++] = frame->rtr ? (ext ? 'R' : 'r') : (ext ? 'T' : 't');
    put_hex(&out[pos], frame->id, ext ? 8 : 3);
    pos += ext ? 8 : 3;
    out[pos++] = hex_digits[frame->len];
    if (!frame->rtr)
    {
        for (size_t i = 0; i < frame->len; i++)
        {
            put_hex(&out[pos], frame->payload[i], 2);
            pos += 2;
        }
    }
    out[pos++] = '\r';
    return pos;
}
