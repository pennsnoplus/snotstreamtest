typedef struct {
    uint16_t packet_num;
    uint8_t packet_type;
    uint8_t num_bundles;
} XL3_CommandHeader;

typedef struct {
    XL3_CommandHeader cmdHeader;
	char payload[1440];
} XL3_Packet; // 1444 bytes = 1440 (payload) + 4 (header)

typedef struct {
    uint32_t word1;
    uint32_t word2;
    uint32_t word3;
} PMTBundle;

typedef struct caenPacket { // CAEN data
	char head[4];
	char body[1760];
} caenPacket;

typedef struct mtcPacket { // MTC data
	char head[4];
	char body[192];
} mtcPacket;
