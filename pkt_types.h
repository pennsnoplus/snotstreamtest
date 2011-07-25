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

/*
   Different types of monitoring connections
 */
typedef enum {
	EV_BUILDER, // TODO: EV_BUILDER data
	XL3,
	MTC,
	CAEN,
	ORCA,       // TODO: ORCA data
    UNKNOWN,    // TODO: default size
	con_type_max
} con_type;
/*
   Sizes of different packet types
   (con_size_of[EV_BUILDER]);
 */
int con_size_of = {
    0,
    sizeof xl3Packet,
    sizeof mtcPacket,
    sizeof caenPacket,
    0,
    0
}
