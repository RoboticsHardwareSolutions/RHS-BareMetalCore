#include "core/log.h"
#include "mongoose.h"

#define TAG "mongoose"

static int   s_level = MG_LL_INFO;
static const char* s_file = NULL;
static int   s_line = 0;
static const char* s_func = NULL;


static const char* get_basename(const char* path)
{
    const char* last_slash = NULL;
    for (const char* p = path; *p; ++p)
    {
        if (*p == '/' || *p == '\\')
            last_slash = p;
    }
    return (last_slash != NULL) ? last_slash + 1 : path;
}


void mg_log_prefix(int ll, const char* file, int line, const char* fname)
{
    s_level = ll;
    s_file = get_basename(file);
    s_line = line;
    s_func = fname;
}


void mg_log(const char* fmt, ...)
{
    va_list ap;
    char   buf[256];
    size_t len;

    (void) len;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (s_file != NULL)
    {
        len = strlen(buf);
        if (len + 32 < sizeof(buf))
        {
            snprintf(&buf[len], sizeof(buf) - len, " (%s:%d %s)", s_file, s_line, s_func);
        }
    }

    switch (s_level)
    {
    case MG_LL_ERROR:   RHS_LOG_E(TAG, "%s", buf); break;
    case MG_LL_INFO:    RHS_LOG_I(TAG, "%s", buf); break;
    case MG_LL_DEBUG:   RHS_LOG_D(TAG, "%s", buf); break;
    case MG_LL_VERBOSE: RHS_LOG_T(TAG, "%s", buf); break;
    default:            RHS_LOG_I(TAG, "%s", buf); break;
    }
}



