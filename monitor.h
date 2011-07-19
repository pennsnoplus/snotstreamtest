/* Defines */
#define MAXBUFSIZE 1764
#define NUM_CONS 3 // maximum number of ports to be monitored
#define CONT_PORT 7999 // controller port
#define MAXHOSTSIZE 100
#define MAXCONS 5
/* Structs */
typedef void(*com_ptr)(char *);
typedef enum {
    // different types of connections
	CONTROLLER,
	EV_BUILDER,
	XL3,
	MTC,
	CAEN,
	ORCA,
	con_type_max
} con_type;

typedef struct {
	// Holds the data for a single monitoring connection
	struct bufferevent *bev; // holds a buffer event
	char host[MAXHOSTSIZE];
	int port;
	con_type type;
} data_con;

typedef struct {
    // holds a list of commands
    char *key;
    com_ptr function;
} command;

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
/* Prototypes */
