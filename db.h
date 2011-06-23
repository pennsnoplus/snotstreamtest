#include "penn_daq.h"
#include "pillowtalk.h"

#ifndef DB_H
#define DB_H


typedef struct {
  /* index definitions [0=ch0-3; 1=ch4-7; 2=ch8-11, etc] */
  byte     rmp[8];    // back edge timing ramp    
  byte     rmpup[8];  // front edge timing ramp    
  byte     vsi[8];    // short integrate voltage     
  byte     vli[8];    // long integrate voltage
} tdisc_t;

typedef struct {
  /* the folowing are motherboard wide constants */
  byte     vmax;           // upper TAC reference voltage
  byte     tacref;         // lower TAC reference voltage
  byte     isetm[2];       // primary   timing current [0= tac0; 1= tac1]
  byte     iseta[2];       // secondary timing current [0= tac0; 1= tac1]
  /* there is one byte of TAC bits for each channel */
  byte     tac_shift[32];  // TAC shift register load bits (see 
                           // loadcmosshift.c for details)
                           // [channel 0 to 31]      
} tcmos_t;

/* CMOS shift register 100nsec trigger setup */
typedef struct {
  byte      mask[32];   
  byte      tdelay[32];    // tr100 width (see loadcmosshift.c for details)
                           // [channel 0 to 31], only bits 0 to 6 defined
} tr100_t;

/* CMOS shift register 20nsec trigger setup */
typedef struct {
  byte      mask[32];    
  byte      twidth[32];    //tr20 width (see loadcmosshift.c for details)
                           // [channel 0 to 31], only bits 0 to 6 defined
  byte      tdelay[32];    //tr20 delay (see loadcmosshift.c for details)
                           // [channel 0 to 31], only buts 0 to 4 defined
} tr20_t;

typedef struct {
  uint16_t mb_id;
  uint16_t dc_id[4];
  byte vbal[2][32];
  byte vthr[32];
  tdisc_t tdisc;
  tcmos_t tcmos;
  byte vint;       // integrator output voltage 
  byte hvref;     // MB control voltage 
  tr100_t tr100;
  tr20_t tr20;
  uint16_t scmos[32];     // remaining 10 bits (see loadcmosshift.c for 
  uint32_t  disable_mask;
} mb_t;

typedef struct {
  uint16_t lockout_width;
  uint16_t pedestal_width;
  uint16_t nhit100_lo_prescale;
  uint32_t pulser_period;
  uint32_t low10Mhz_clock;
  uint32_t high10Mhz_clock;
  float fine_slope;
  float min_delay_offset;
  uint16_t coarse_delay;
  uint16_t fine_delay;
  uint32_t gt_mask;
  uint32_t gt_crate_mask;
  uint32_t ped_crate_mask;
  uint32_t control_mask;
}mtcd_t;

typedef struct{
    char id[20];
    uint16_t threshold;
    uint16_t mv_per_adc;
    uint16_t mv_per_hit;
    uint16_t dc_offset;
}trigger_t;

typedef struct {
  trigger_t triggers[14];
  uint16_t crate_status[6];
  uint16_t retriggers[6];
}mtca_t;

typedef struct {
    mtcd_t mtcd;
    mtca_t mtca;
    uint32_t tub_bits;
}mtc_t;

int parse_fec_hw(pt_node_t* value,mb_t* mb);
int parse_fec_debug(pt_node_t* value,mb_t* mb);
int parse_test(pt_node_t* test,mb_t* mb,int cbal,int zdisc,int all);
int swap_fec_db(mb_t* mb);
int parse_mtc(pt_node_t* value,mtc_t* mtc);
int get_new_id(char* newid);
int post_debug_doc(int crate, int card, pt_node_t* doc);
int post_debug_doc_with_id(int crate, int card, char *id, pt_node_t* doc);
#endif

