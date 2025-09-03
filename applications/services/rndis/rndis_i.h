#include "rndis.h"
#include "rhs.h"

// Helper macro for MAC generation
#define GENERATE_LOCALLY_ADMINISTERED_MAC(UUID) \
    {2,                                         \
     UUID[0] ^ UUID[1],                         \
     UUID[2] ^ UUID[3],                         \
     UUID[4] ^ UUID[5],                         \
     UUID[6] ^ UUID[7] ^ UUID[8],               \
     UUID[9] ^ UUID[10] ^ UUID[11]}
