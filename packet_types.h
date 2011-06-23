#define MAXBUFSIZE 1764

typedef struct caenPacket { // CAEN data
	char head[4];
	char body[1760];
} caenPacket;

typedef struct mtcPacket { // MTC data
	char head[4];
	char body[192];
} mtcPacket;

typedef struct xl3Packet { // XL3 data
	char head[4];
	char body[1440];
} xl3Packet;
