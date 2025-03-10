#include "loader.h"
#include "rhs.h"

typedef struct {
    uint8_t type;
} LoaderMessage;

struct Loader {
    RHSMessageQueue* queue;
};
