// This file reads in the C-struct packet format
// and runs it through the simulated electronics
// readout to reorder it and turn it into various
// packet streams. These streams are then duplicated
// until you have a sufficiently large set of premade
// packets. It then writes the packets out to files.
// there will be a separate file for each xl3 and sbc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "shipper.h"
#include "ds.h"
#include "listener.h"


#define NUM_CHANS 19*16*32

#define PER_CHANNEL 10 
//#define PER_FEC (32*PER_CHANNEL)
#define PER_FEC (32*50)
#define PER_CRATE (304*PER_FEC)
#define INPUT_FILE "pbomb10kclean.txt"

uint32_t get_bits(uint32_t x, uint32_t position, uint32_t count)
{
    uint32_t shifted = x >> position;
    uint32_t mask = ((uint64_t)1 << count) -1;
    return shifted & mask;
}

uint32_t pmtbundle_pmtid(PMTBundle* p)
{
    int ichan = get_bits(p->word[0],16,5);
    int icard = get_bits(p->word[0],26,4);
    int icrate = get_bits(p->word[0],21,5);
    return (512*icrate + 32*icard + ichan);
}

uint32_t pmtbundle_gtid(PMTBundle* p)
{
    uint32_t gtid1 = get_bits(p->word[0],0,16);
    uint32_t gtid2 = get_bits(p->word[2],12,4);
    uint32_t gtid3 = get_bits(p->word[2],28,4);
    return (gtid1 + (gtid2<<16) + (gtid3<<20));
}

int main(int argc, char *argv[])
{
    int i,j;
    int icrate,ifec;
    int num_events;
    char ibuf[10000];
    int last_gtid1[10000];
    int last_gtid2[10000];
    int last_gtid3[10000];
    char *words;
    int crate,card,channel,chan_id;
    uint32_t gtid,current_gtid;
    PMTBundle *fecs[304];
    PMTBundle *xl3[19]; 
    XL3Packet packet;
    int xreadptr[19],xwriteptr[19];
    uint16_t data_avail[19];
    int current_slot[19];

    int freadptr[304],fwriteptr[304];
    int sequencer[304];
    FILE *xl3file[19];

    // initialization
    srand(time(NULL));
    int done = 0;
    int events_in_file = 0;
    memset((char *) last_gtid1, 0, 10000*sizeof(int));
    memset((char *) last_gtid2, 0, 10000*sizeof(int));
    memset((char *) last_gtid3, 0, 10000*sizeof(int));
    for (i=0;i<19;i++){
        data_avail[i] = 0x0;
        current_slot[i] = 0;
        xl3[i]  = (PMTBundle *) malloc(PER_CRATE*sizeof(PMTBundle));
        xreadptr[i] = 0;
        xwriteptr[i] = 0;
        char fname[200];
        sprintf(fname,"data/xl3%02d.dat",i);
        xl3file[i] = fopen(fname,"w");
    }

    // channel by channel analog storage
    PMTBundle *channels[NUM_CHANS];
    int creadptr[NUM_CHANS],cwriteptr[NUM_CHANS];
    for (i=0;i<NUM_CHANS;i++){
        creadptr[i] = 0;
        cwriteptr[i] = 0;
        channels[i] = (PMTBundle *) malloc(PER_CHANNEL*sizeof(PMTBundle));
    }
    for (i=0;i<10;i++){
        for (j=0;j<9728;j++){
            channels[j][i].word[0] = 0x0;
            channels[j][i].word[1] = 0x0;
            channels[j][i].word[2] = 0x0;
        }
    }

    for (i=0;i<304;i++){
        freadptr[i]=0;
        fwriteptr[i]=0;
        sequencer[i]=0;
        fecs[i] = (PMTBundle *) malloc(PER_FEC*sizeof(PMTBundle));
    }
    for (i=0;i<PER_FEC;i++){
        for (j=0;j<304;j++){
            fecs[j][i].word[0] = 0x0;
            fecs[j][i].word[1] = 0x0;
            fecs[j][i].word[2] = 0x0;
        }
    }

    // OK we will eventually be reading in a file with
    // the C-struct type packed format, for now we are
    // reading in ascii
    FILE *infile;
    infile = fopen(INPUT_FILE,"r");


    while (done < 4)
    {
	printf("done = %d\n",done);
        // keep looping here until we have read in every event
        // in the file
        if (done == 0){
            // the first thing that happens in the detector when
            // an event occurs is that the analog signals get stored
            // in each channels own analog storage. We will here add
            // a random number of events into analog storage.
            // We read in a whole number of events at a time because
            // the sequencer reads out much slower than events will
            // come in, so the entire event will be in analog memory
            // by the time any hit is read out by a sequencer

            // read in from 1 to 5 events
            num_events = rand()%5+1;
            for (i=0;i<num_events;i++){
                if (fgets(ibuf,10000,infile)){
                    events_in_file++;
                    current_gtid = 0xFFFFFFFF;

                    // this is all temp for the ascii version
                    words = strtok(ibuf, " ");
                    PMTBundle temp_bundle;
                    int num_hits = atoi(words);
                    for (j=0;j<num_hits;j++){
                        words = strtok(NULL, " ");
                        temp_bundle.word[0] = strtoul(words,(char**)NULL,16);
                        words = strtok(NULL, " ");
                        temp_bundle.word[1] = strtoul(words,(char**)NULL,16);
                        words = strtok(NULL, " ");
                        temp_bundle.word[2] = strtoul(words,(char**)NULL,16);

                        crate = (temp_bundle.word[0] & 0x03E00000) >> 21;
                        card = (temp_bundle.word[0] & 0x3C000000) >> 26;
                        channel = (temp_bundle.word[0] & 0x001F0000) >> 16;
                        gtid = pmtbundle_gtid(&temp_bundle);
                        chan_id = crate*16*32+card*32+channel;

                        // check that our gtids are coming in order
                        if (current_gtid > 0xFFFFFF){
                            current_gtid = gtid;
                        }else if (gtid != current_gtid){
                            printf("!!!Event %d - hit %d different gtid! %u,%u\n",
                                    events_in_file,j,gtid,current_gtid);
                        }else{
                            //    printf("Event %d - hit %d - %u,%u\n",
                            //            events_in_file,j,gtid,current_gtid);
                        }

                        // check that gtids are in order by channel
                        if (gtid > last_gtid1[chan_id] || gtid == 0)
                            last_gtid1[chan_id] = gtid;
                        else
                            printf("Out of order to analog memory for pmt %d, %d < %d\n",chan_id,gtid,last_gtid1[chan_id]); 

                        channels[chan_id][cwriteptr[chan_id]].word[0] = temp_bundle.word[0];
                        channels[chan_id][cwriteptr[chan_id]].word[1] = temp_bundle.word[1];
                        channels[chan_id][cwriteptr[chan_id]].word[2] = temp_bundle.word[2];
                        cwriteptr[chan_id]++;
                        if (cwriteptr[chan_id] == PER_CHANNEL){
                            cwriteptr[chan_id] = 0;
                        }else if(cwriteptr[chan_id] == creadptr[chan_id]){
                            printf("error writing channel %d, %u %u\n",chan_id,cwriteptr[chan_id],creadptr[chan_id]);
                        }
                    } // end loop over hits
                }else{ // when no more events in file
                    done = 1;
                }
            } // end loop over random number of events being buffered
        } // end if done == 0





        // now we simulate the sequencer. It will loop through a random number
        // of channels and read them out into the fec fifo buffers

        if (done < 2){
            int ifec;
            for (ifec=0;ifec<304;ifec++){        
                // each fec does its own sequencer readout loop
                card = ifec%16;
                crate = (ifec-card)/16;
                num_events = rand()%(5*32)+1; // it goes around up to five full loops
                if (crate == 17 && card == 15)
                    num_events = 5*32;
                for (i=0;i<num_events;i++){
                    // each sequencer reads out the channels in order
                    chan_id = ifec*32+sequencer[ifec];

                    // check if there is something in this
                    // channels analog memory
                    if (creadptr[chan_id] != cwriteptr[chan_id]){
                        fecs[ifec][fwriteptr[ifec]].word[0] = channels[chan_id][creadptr[chan_id]].word[0];
                        fecs[ifec][fwriteptr[ifec]].word[1] = channels[chan_id][creadptr[chan_id]].word[1];
                        fecs[ifec][fwriteptr[ifec]].word[2] = channels[chan_id][creadptr[chan_id]].word[2];

                        // check that gtids are coming in order
                        int gtid = pmtbundle_gtid(&fecs[ifec][fwriteptr[ifec]]);
                        fwriteptr[ifec]++;
                        if (gtid > last_gtid2[chan_id] || gtid == 0)
                            last_gtid2[chan_id] = gtid;
                        else
                            printf("out of order into fec buffer, pmt %d, %d < %d\n",chan_id,gtid,last_gtid2[chan_id]); 


                        if (fwriteptr[ifec] == PER_FEC)
                            fwriteptr[ifec] = 0;
                        else if (fwriteptr[ifec] == freadptr[ifec])
                            printf("error writing card %d, %u %u\n",ifec,fwriteptr[ifec],freadptr[ifec]);
                        creadptr[chan_id]++;
                        if (creadptr[chan_id] == PER_CHANNEL)
                            creadptr[chan_id] = 0;
                    } // end if readptr != writeptr
                    sequencer[ifec] = (sequencer[ifec]+1)%32;
                } // end loop over number of sequencer reads
            } // end loop over fecs

            if (done == 1){
                for (i=0;i<9728;i++){
                    if (creadptr[i] == cwriteptr[i]){
                        done = 2; // we are done with file and channels
                    }else{
                        done = 1;
                        break;
                    }
                }
            }
        } // end done < 2


        if (done < 3){
            // now we simulate the readout of the fecs by the xl3.
            // we will do a random number of fec reads. every time
            // it gets to the last fec, it scans all the fec memories
            // to see who needs to be read out. it then loops through
            // those fecs and reads out up to 120 bundles from each.
            for (icrate=0;icrate<19;icrate++){ // loop over crates
                int num_fecs = rand()%(2*16)+1; // it goes around up to two full loops
                for (i=0;i<num_fecs;i++){

                    // if we are back at the beginning we should
                    // repopulate data_available
                    if (current_slot[icrate] == 0){
                        data_avail[icrate] = 0x0;
                        for (j=0;j<16;j++){
                            if (freadptr[icrate*16+j] != fwriteptr[icrate*16+j]){
                                data_avail[icrate] |= 0x1<<j; // fill data available
                            }
                        }
                    }

                    // check if this slot has data
                    if ((0x1<<current_slot[icrate]) & data_avail[icrate]){
                        // if so read up to 120 bundles
                        card = icrate*16+current_slot[icrate];
                        for (j=0;j<120;j++){
                            // check if we are done
                            if (freadptr[card] ==
                                    fwriteptr[card]){
                                data_avail[icrate] &= ~(0x1<<current_slot[icrate]);
                                break;
                            }
                            xl3[icrate][xwriteptr[icrate]].word[0] = fecs[card][freadptr[card]].word[0];
                            xl3[icrate][xwriteptr[icrate]].word[1] = fecs[card][freadptr[card]].word[1];
                            xl3[icrate][xwriteptr[icrate]].word[2] = fecs[card][freadptr[card]].word[2];

                            gtid = pmtbundle_gtid(&xl3[icrate][xwriteptr[icrate]]);
                            chan_id = pmtbundle_pmtid(&xl3[icrate][xwriteptr[icrate]]); 
                            if (gtid > last_gtid3[chan_id] || gtid == 0)
                                last_gtid3[chan_id] = gtid;
                            else
                                printf("out of order for pmt %d into xl3, %d < %d\n",chan_id,gtid,last_gtid3[chan_id]); 
                                //printf("fecs[%d][freadptr[%d](%d)]->gtid = %d\n",card,card,freadptr[card],pmtbundle_gtid(&fecs[card][freadptr[card]]));
                            freadptr[card]++;
                            if (freadptr[card] == PER_FEC)
                                freadptr[card] = 0;
                            xwriteptr[icrate]++;
                            if (xwriteptr[icrate] == PER_CRATE)
                                xwriteptr[icrate] = 0;
                            if (xwriteptr[icrate] == xreadptr[icrate])
                                printf("error writing xl3 %d, %u %u\n",icrate,xwriteptr[icrate],xreadptr[icrate]);

                        } // end loop over 120 bundles
                    } // end if slot has data
                    current_slot[icrate]++;
                    current_slot[icrate] = current_slot[icrate]%16;
                } // end loop over num_fecs
            } // end loop over crates
        } // end done < 3
        if (done == 2){
            for (i=0;i<304;i++){
                if (freadptr[i] == fwriteptr[i]){
                    done = 3; // we are done with file and channels
                }else{
                    done = 2;
                    break;
                }
            }
        }


        if (done < 4){
            // now we will write out each xl3 buffer into packets
            // Each packet is up to 120 bundles. 
            // We read out all of the bundles we have
            for (icrate=0;icrate<19;icrate++){
                while (xwriteptr[icrate] != xreadptr[icrate]){
                    int num_bundles = 0;
                    // lets make a packet
                    packet.cmdHeader.packet_num = 0xABCD; //FIXME
                    packet.cmdHeader.packet_type = 0xCC; //FIXME
                    // write out up to 120 bundles
                    for (i=0;i<MEGASIZE;i++){
                        // stop if done with this xl3
                        if (xwriteptr[icrate] == xreadptr[icrate])
                            break;
                        
                        PMTBundle *bundle = (PMTBundle *) (packet.payload + num_bundles*sizeof(PMTBundle));
                        bundle->word[0] = xl3[icrate][xreadptr[icrate]].word[0];
                        bundle->word[1] = xl3[icrate][xreadptr[icrate]].word[1];
                        bundle->word[2] = xl3[icrate][xreadptr[icrate]].word[2];
                        num_bundles++;
                        xreadptr[icrate]++;
                    }

		    packet.cmdHeader.num_bundles = num_bundles;
		    int n;
		    for (n=0;n<10;n++){
			printf("%08x ",*(((uint32_t *) &packet)+n));
		    }
		    printf("\n");
		    fwrite(&packet, sizeof(packet), 1, xl3file[icrate]);

		} // end while xl3 not empty
	    } // end loop over crates
	} // end if done < 4

	if (done == 3){
	    for (i=0;i<19;i++){
		if (xwriteptr[i] == xreadptr[i]){
		    done = 4;
		}else{
		    done = 3;
		    break;
		}
	    }
	}

    }

    for (i=0;i<19;i++){
	fclose(xl3file[i]);
	free(xl3[i]);
    }
    for (i=0;i<304;i++){
	free(fecs[i]);
    }
    for (i=0;i<NUM_CHANS;i++){
	free(channels[i]);
    }

    fclose(infile);
    return 0;
}

