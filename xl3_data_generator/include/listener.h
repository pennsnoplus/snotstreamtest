#define NUM_THREADS 20
#define HEADER_LENGTH 6
#define MEGASIZE 120
#define XL3_MAXPAYLOADSIZE_BYTES (MEGASIZE*12) 
#define MAX_BUFFER_LEN (XL3_MAXPAYLOADSIZE_BYTES+HEADER_LENGTH)
#define NCRATES 19
#define NFECS 16
#define NCHANS 32 

int sockfd, thread_sockfd[NUM_THREADS];
void close_sockets();
void handler(int signal);
void die(const char *msg);
void* listener_child(void* psock);
void* listener(void* ptr);

uint16_t read_pos[NCRATES];
uint32_t seq_pos[NCRATES][NFECS];
uint32_t crate_gtid_current[NCRATES]; // document these variables
uint32_t crate_gtid_last[NCRATES];    // before you forget what they are
uint16_t crate_skipped[NCRATES];

void accept_xl3packet(void* packet_buffer);

typedef enum {
    XL3_PACKET,
    MTC_PACKET,
    CAEN_PACKET,
    TRIG_PACKET,
    EPED_PACKET,
    RHDR_PACKET,
    CAST_PACKET,
    CAAC_PACKET
} PacketType;

typedef struct
{
    uint16_t type;
} PacketHeader;

typedef struct
{
  uint16_t packet_num;
  uint8_t packet_type;
  uint8_t num_bundles;
} XL3_CommandHeader;

typedef struct
{
  XL3_CommandHeader cmdHeader;
  char payload[XL3_MAXPAYLOADSIZE_BYTES];
} XL3Packet;

