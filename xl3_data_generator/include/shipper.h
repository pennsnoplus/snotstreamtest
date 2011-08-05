#define MAX_RHDR_WAIT 100000*50

// CDABHeader: a header that precedes each structure in a CDAB file
typedef struct {
    uint32_t record_type;
    uint32_t size;
} CDABHeader;

void* shipper(void* ptr);

