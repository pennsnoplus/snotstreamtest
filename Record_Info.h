
//  This file was taken verbatim from University oF Washington code and 
//  subsequently modified.

/*  Record_Info.h 
 *  defines structures of Records for SNO */
#ifndef RECORD_INFO


#ifndef lint
static char RECORD_INFO_H_ID[] = "$Id: Record_Info.h,v 1.19 1997/12/21 23:31:28 wittich Exp $";
#endif // lint


/*  unpacking "routines" by definition! Looks awful, but is fast...
 *  "a" in the following is a pointer to 3 longwords as read out from
 *  the FEC32.  from SNODAQ distribution */
#define UNPK_MISSED_COUNT(a)    ( (*(a+1) >> 28) & 0x1 )
#define UNPK_NC_CC(a)           ( (*(a+1) >> 29) & 0x1 )
#define UNPK_LGI_SELECT(a)      ( (*(a+1) >> 30) & 0x1 )
#define UNPK_CMOS_ES_16(a)      ( (*(a+1) >> 31) & 0x1 )
#define UNPK_CGT_ES_16(a)       ( (*(a) >> 30) & 0x1 )
#define UNPK_CGT_ES_24(a)       ( (*(a) >> 31) & 0x1 )
#define MY_UNPK_QLX(a)   ( (*(a+1) & 0x7FFUL) | ( ~ (*(a+1)) & 0x800UL))
#define UNPK_QLX(a)     ( (*(a+1) & 0x00000800) == 0x800 ? \
                          (*(a+1) & 0x000007FF) : \
                          (*(a+1) & 0x000007FF) + 2048 )
#define UNPK_QHS(a)     ( ((*(a+1) >>16) & 0x00000800) == 0x800 ? \
                          ((*(a+1)>>16) & 0x000007FF) : \
                          ((*(a+1) >>16) & 0x000007FF) + 2048 )
#define UNPK_QHL(a)     ( (*(a+2) & 0x00000800)  == 0x800 ? \
                          (*(a+2) &0x000007FF) : (*(a+2) & 0x000007FF) + 2048 )
#define UNPK_TAC(a)     ( ((*(a+2) >>16) & 0x00000800)  == 0x800 ? \
                          ((*(a+2)>>16) & 0x000007FF) : \
                          ((*(a+2) >>16) & 0x000007FF) + 2048 )
#define UNPK_CELL_ID(a)         ( (*(a+1) >> 12) & 0x0000000F )
#define UNPK_CHANNEL_ID(a)      ( (*(a) >> 16) & 0x0000001F )
#define UNPK_BOARD_ID(a)        ( (*(a) >> 26) & 0x0000000F )
#define UNPK_CRATE_ID(a)        ( (*(a) >> 21) & 0x0000001F )
#define UNPK_FEC_GT24_ID(a)  ( *(a) & 0x0000FFFF ) | \
                                ( (*(a+2) << 4) &0x000F0000 )  | \
                                ( (*(a+2) >> 8) & 0x00F00000 )
#define UNPK_FEC_GT16_ID(a)  ( *(a) & 0x0000FFFF ) 
#define UNPK_FEC_GT8_ID(a)   ( (*(a+2) >> 24) &0x000000F0 ) | \
                                ( (*(a+2) >> 12) & 0x0000000F )

#ifdef NO_RUSHDY_BOARDS
#       define UNPK_FEC_GT_ID(a)    ( *(a) & 0x0000FFFF ) //this is for Peter
                                  // (when CGT was fubar'd)
#else                                  
#       define UNPK_FEC_GT_ID(a)    ( *(a) & 0x0000FFFF ) | \
                                  ( (*(a+2) << 4) &0x000F0000 )  | \
                                  ( (*(a+2) >> 8) & 0x00F00000 )
#endif

//   Record ID's 
#define RUN_RECORD     0x52554E20 // 'RUN '
#define PMT_RECORD     0x504d5420 // 'PMT '
#define EPED_RECORD    0x45504544 // 'EPED'
#define TRIG_RECORD    0x54524947 // 'TRIG'

/*  version numbers...
 *  as of Feb 1997 (from UW) 
 */
#define DAQ_CODE_VERSION             0
#define RUN_RECORD_VERSION           0
#define PMT_RECORD_VERSION           0
#define TRIG_RECORD_VERSION          0

#define PTK_RECORD_VERSIONS  0x50544B30 // 'PTK0'

#ifdef UW_FUBARED
#define CONFIG_RECORD 		 'CFIG' 
#define SOURCE_POSITION_RECORD	 'SRCP' 
#define LASER_POSITION_RECORD	 'LSRP' 
#define ACRYLIC_POSITION_RECORD	 'ACRP' 

#endif // UW_FUBARED

//   master trigger card information valid for entire run 
typedef struct TriggerInfo {
  uint32_t        TriggerMask;	  //  which triggers were set? 
  uint32_t        n100lo;    // trigger Threshold settings
  uint32_t        n100med;   // these are longs cuz Josh is a weenie.
  uint32_t        n100hi;
  uint32_t        n20;
  uint32_t        n20lb;
  uint32_t        esumlo;
  uint32_t        esumhi;
  uint32_t        owln;
  uint32_t        owlelo;
  uint32_t        owlehi;
  uint32_t        n100lo_zero;    // trigger Threshold zeroes
  uint32_t        n100med_zero;
  uint32_t        n100hi_zero;
  uint32_t        n20_zero;
  uint32_t        n20lb_zero;
  uint32_t        esumlo_zero;
  uint32_t        esumhi_zero;
  uint32_t        owln_zero;
  uint32_t        owlelo_zero;
  uint32_t        owlehi_zero;
  uint32_t PulserRate;	    //  MTC local pulser 
  uint32_t ControlRegister;  //  MTC control register status 
  uint32_t reg_LockoutWidth; //  min. time btwn global triggers 
  uint32_t reg_Prescale;     //  how many nhit_100_lo triggers to take 
  uint32_t GTID;             //  to keep track of where I am in the world
} aTriggerInfo, *aTriggerInfoPtr;




//   define the masks 
//   Run Mask... 
#define NEUTRINO_RUN        0x0001
#define SOURCE_RUN          0x0002
#define CALIB_RUN           0x0004
#define NCD_RUN             0x0008
#define SALT_RUN            0x0010
#define POISON_RUN          0x0020
#define PARTIAL_FILL_RUN    0x0040
#define AIR_FILL_RUN        0x0080
#define D2O_RUN             0x0100
#define H2O_RUN             0x0200
#define MINISNO_RUN         0x0400
//   this still leaves 21 bits 

//   Source Mask... 
#define NO_SRC                  0x10000001UL // testing, for now
#define ROTATING_SRC            0x00001
#define LASER_SRC               0x00002
#define SONO_SRC                0x00004
#define N16_SRC                 0x00008
#define N17_SRC                 0x00010
#define NAI_SRC                 0x00020
#define LI8_SRC                 0x00040
#define PT_SRC                  0x00080
#define CF_HI_SRC               0x00100
#define CF_LO_SRC               0x00200
#define U_SRC                   0x00400
#define TH_SRC                  0x00800
#define P_LI7_SRC               0x01000
#define WATER_SAMPLER           0x02000
#define PROP_COUNTER_SRC        0x04000
#define SINGLE_NCD_SRC          0x08000
#define SELF_CALIB_SRC          0x10000
//   this leaves 15 bits for future/forgotten sources. 

#ifdef UW_FUBARED

//   trigger mask, for use with UNPK_MTC_TRIGGER 
#define OWLE_LO                 0x00000001
#define OWLE_HI                 0x00000002
#define PULSE_GT                0x00000004
#define PRESCALE                0x00000008
#define PEDESTAL                0x00000010
#define PONG                    0x00000020
#define SYNC                    0x00000040
#define EXT_ASYNC               0x00000080
#define EXT2                    0x00000100
#define EXT3                    0x00000200
#define EXT4                    0x00000400
#define EXT5                    0x00000800
#define EXT6                    0x00001000
#define EXT7                    0x00002000
#define EXT8                    0x00004000
#define SPECIAL_RAW             0x00008000
#define NCD                     0x00010000
#define SOFT_GT                 0x00020000
#define MISS_TRIG               0x00040000
#define NHIT_100_LO             0x00080000
#define NHIT_100_MED            0x00100000
#define NHIT_100_HI             0x00200000
#define NHIT_20                 0x00400000
#define NHIT_20_LB              0x00800000
#define ESUM_LO                 0x01000000
#define ESUM_HI                 0x02000000
#define OWLN                    0x04000000

//   trigger error mask 
#define TESTGT                  0x00000001
#define TEST50                  0x00000002
#define TEST10                  0x00000004
#define TESTMEM1                0x00000008
#define TESTMEM2                0x00000010
#define SYNCLR16                0x00000020
#define SYNCLR16_WO_TC16        0x00000040
#define SYNCLR24                0x00000080
#define SYNCLR24_WO_TC24        0x00000100
#define SOME_FIFOS_EMPTY        0x00000200
#define SOME_FIFOS_FULL         0x00000400
#define ALL_FIFOS_FULL          0x00000800

//   electronics calibration type 
#define T_PEDESTALS             0x0001
#define T_SLOPES                0x0002
#define Q_PEDESTALS             0x0004
#define Q_SLOPES                0x0008

#endif //   UW_FUBARED  

//   FEC data as read out in 96-bit structure 
typedef struct FECReadoutData {
  //   word 1 (starts from MSB): 
  unsigned CGT_ES24             :1;
  unsigned CGT_ES16             :1;
  unsigned BoardID              :4;
  unsigned CrateID              :5;
  unsigned ChannelID            :5;
  unsigned GTID1                :16; //   lower 16 bits 
  //   word 2: 
  unsigned Cmos_ES16            :1;
  unsigned LGI_Select           :1;
  unsigned NC_CC                :1;
  unsigned MissedCount          :1;
  unsigned SignQhs              :1;
  unsigned Qhs                  :11;
  unsigned CellID               :4;
  unsigned SignQlx              :1;
  unsigned Qlx                  :11;
  //   word 3            : 
  unsigned GTID3                :4;		//   bits 21-24 
  unsigned SignTAC              :1;
  unsigned TAC                  :11;
  unsigned GTID2                :4;		//   bits 17-20 
  unsigned SignQhl              :1;
  unsigned Qhl                  :11;
} aFECReadoutData, *aFECReadoutDataPtr;

//   Master Trigger Card data 
typedef struct MTCReadoutData {
  //   word 0 
  uint32_t Bc10_1          :32;
  //   word 1 
  uint32_t Bc50_1          :11;
  uint32_t Bc10_2          :21;
  //   word 2 
  uint32_t Bc50_2          :32;
  //   word 3 
  unsigned Owln                 :1; //   MSB 
  unsigned ESum_Hi              :1;
  unsigned ESum_Lo              :1;
  unsigned Nhit_20_LB           :1;
  unsigned Nhit_20              :1;
  unsigned Nhit_100_Hi          :1;
  unsigned Nhit_100_Med         :1;
  unsigned Nhit_100_Lo          :1;
  uint32_t BcGT            :24; //   LSB 
  //   word 4 
  unsigned Diff_1               :3;
  unsigned Peak                 :10;
  unsigned Miss_Trig            :1;
  unsigned Soft_GT              :1;
  unsigned NCD_trigger          :1;
  unsigned Special_Raw          :1;
  unsigned Ext_2                :1;
  unsigned Ext_3                :1;
  unsigned Ext_4                :1;
  unsigned Ext_5                :1;
  unsigned Ext_6                :1;
  unsigned Ext_7                :1;
  unsigned Ext_8                :1;
  unsigned Ext_Async            :1;
  unsigned Sync                 :1;
  unsigned Pong                 :1;
  unsigned Pedestal             :1;
  unsigned Prescale             :1;
  unsigned Pulse_GT             :1;
  unsigned Owle_Hi              :1;
  unsigned Owle_Lo              :1;
  //  word 5 
  unsigned Unused3              :1;
  unsigned Unused2              :1;
  unsigned Unused1              :1;
  unsigned FIFOsAllFull         :1;
  unsigned FIFOsNotAllFull      :1;
  unsigned FIFOsNotAllEmpty     :1;
  unsigned SynClr24_wo_TC24     :1;
  unsigned SynClr24             :1;
  unsigned SynClr16_wo_TC16     :1;
  unsigned SynClr16             :1;
  unsigned TestMem2             :1;
  unsigned TestMem1             :1;
  unsigned Test10               :1;
  unsigned Test50               :1;
  unsigned TestGT               :1;
  unsigned Int                  :10;
  unsigned Diff_2               :7;
} aMTCReadoutData, *aMTCReadoutDataPtr;

//   generic information for every header, preceeds ALL following records! 
typedef struct GenericRecordHeader {
  uint32_t RecordID;
  uint32_t RecordLength;   //  length of record to follow, 
                                // NOT INCLUDING generic record header! 
  uint32_t RecordVersion;
} aGenericRecordHeader;

#define HEADERSIZE  sizeof( aGenericRecordHeader ) // 10 bytes generic header.

//   run record 
typedef struct RunRecord {
  time_t  Time;   // ptk used instead of aDate_Time
  uint32_t DAQCodeVersion;
  uint32_t RunNumber;
  uint32_t CalibrationTrialNumber;
  uint32_t SourceMask;	//   which sources in? 
  uint32_t RunMask;	//   run conditions 
  uint32_t GTCrateMsk;   // this run's GT crate mask
} aRunRecord;

//   trigger record  -- Eped record
typedef struct {
  time_t Time;  // ptk used instead of aDate_Time
  uint16_t CalibrationType;
  u_char halfCrateID; // which 1/2 crate is enabled for pedestals
  u_char reg_PedestalWidth;      // width of pedestal pulse for Qinj 
  u_char reg_Ped_GTDel_Coarse;
  u_char reg_Ped_GTDel_Fine;	//   pedestal delay for T slopes 
  short Qinj_dacsetting;		//   DAC setting for Qinj 
  u_long MTCD_csr;
  u_long GTID;                          // GT Id validity range
  u_long Flag;                          // start/stop flag
} EPEDRecord;

// Calibration types -- note that these are mutually exclusive
#define EPED_Q_SLOPE_RUN        1
#define EPED_T_SLOPE_RUN        2
#define EPED_PED_RUN            3
#define EPED_FIRST_HALF         (u_char) 0
#define EPED_SECOND_HALF        (u_char) 0x80U
#define EPED_START_CAL          1ul       // start of cali run
#define EPED_CHANGE_CAL         2ul       // change of same
#define EPED_STOP_CAL           3ul       // stop of same, crate
#define EPED_END_CAL            4ul       // end of run, all crates


/* 
 * pmt record -- variable-length record containing npmt*3 LW of FEC data.
 * note: Generic Header length indicates only length of the pmt header, and
 * DOES NOT include the hits! 
 *
 * Also includes the following info, from ref_packer_zdab_pmt.f
 *  
 *      -> BEWARE:  HERE, MSB is 32 ! ! ! ! <-
 *  Also, word number below is SNOMAN WORD NUMBER ONLY!
 *
 *  o  Event Header Record (one per event trigger):
 *                                 Number
 *     Name        WORD  LSB Pos.  of Bits       Description
 *     RECORD_TYPE   1      26       7       Record type (e.g. PMT, NCD, etc).
 *     MC_FLAG       1      25       1       0=Real event, 1= MC event.
 *     VER_NUM       1      17       8       ZDAB_PMT format number number.
 *     DATA_TYPE     1       1      16       Run Type (see id_run_types.doc).
 *     RUN_NUMBER    2       1      32       Run number.
 *     EV_NUMBER     3       1      32       Event number in this Run.
 *     DAQ_STATUS    4      17      16       DAQ status flags.
 *     NHITS         4       1      16       Number of fired PMT's.
 *     PCK_TYPE      5      29       4       MC packing type:
 *                                           0= PMT info only
 *                                           1= 0 plus source bank info
 *                                           2= 1 plus jitter/cerenkov history
 *     CAL_TYPE      5      25       4       MC Calibration type:
 *                                           0= simple calibration constants
 *                                           1= full calibration constants
 */

// put flags here, shifted to correct position.  See above.
#define PMT_EVR_RECTYPE         ( 0xA << 9 ) // see SNOMAN Docs for these
#define PMT_EVR_NOT_MC          ( 0x0UL << 8 )
#define PMT_EVR_ZDAB_VER        ( 23UL << 0  )
#define PMT_EVR_DATA_TYPE       0xB 
#define PMT_EVR_DAQ_STAT        0xA 
#define PMT_EVR_PCK_TYPE        ( 0x0 << 28 )
#define PMT_EVR_CAL_TYPE        ( 0x1 << 24)

typedef struct PmtEventRecord {
  uint16_t PmtEventRecordInfo;
  uint16_t DataType;
  uint32_t RunNumber;
  uint32_t EvNumber;
  uint16_t DaqStatus; // Random number, as far as I know...
  uint16_t NPmtHit;
  uint32_t CalPckType;
  aMTCReadoutData TriggerCardData;	//   6 LW of MTC data 
                                        //   data follows directly. 
} aPmtEventRecord;

#define RECORD_INFO
#endif //  RECORD_INFO 
