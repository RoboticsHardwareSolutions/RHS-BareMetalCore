#include "rhs.h"

void rhs_init(void)
{
    rhs_record_init();
    rhs_thread_init();
    rhs_log_init();
}
