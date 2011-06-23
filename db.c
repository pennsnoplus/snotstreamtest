#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "penn_daq.h"
#include "pillowtalk.h"
#include "db.h"

int get_new_id(char* newid)
{
    char get_db_address[500];
    sprintf(get_db_address,"http://%s:%s/_uuids",DB_ADDRESS,DB_PORT);
    pt_response_t *idresponse = pt_get(get_db_address);
    pt_node_t *newerid = idresponse->root;
    sprintf(newid,"%s",pt_string_get(pt_array_get(pt_map_get(newerid,"uuids"),0)));
}

int parse_mtc(pt_node_t* value,mtc_t* mtc)
{
    int i;
    pt_node_t* mtcd = pt_map_get(value,"mtcd");
    pt_node_t* mtca  = pt_map_get(value,"mtca");
    pt_node_t* nhit = pt_map_get(mtca,"nhit");
    pt_node_t* esum = pt_map_get(mtca,"esum");
    pt_node_t* spare = pt_map_get(mtca,"spare");

    mtc->mtcd.lockout_width = pt_integer_get(pt_map_get(mtcd,"lockout_width"));
    mtc->mtcd.pedestal_width = pt_integer_get(pt_map_get(mtcd,"pedestal_width"));
    mtc->mtcd.nhit100_lo_prescale = pt_integer_get(pt_map_get(mtcd,"nhit100_lo_prescale"));
    mtc->mtcd.pulser_period = pt_integer_get(pt_map_get(mtcd,"pulser_period"));
    mtc->mtcd.low10Mhz_clock = pt_integer_get(pt_map_get(mtcd,"low10Mhz_clock"));
    mtc->mtcd.high10Mhz_clock = pt_integer_get(pt_map_get(mtcd,"high10Mhz_clock"));
    mtc->mtcd.fine_slope = (float) pt_double_get(pt_map_get(mtcd,"fine_slope"));
    mtc->mtcd.min_delay_offset = (float) pt_double_get(pt_map_get(mtcd,"min_delay_offset"));
    mtc->mtcd.coarse_delay = pt_integer_get(pt_map_get(mtcd,"coarse_delay"));
    mtc->mtcd.fine_delay = pt_integer_get(pt_map_get(mtcd,"fine_delay"));
    mtc->mtcd.gt_mask = strtoul(pt_string_get(pt_map_get(mtcd,"gt_mask")),(char**) NULL, 16);
    mtc->mtcd.gt_crate_mask = strtoul(pt_string_get(pt_map_get(mtcd,"gt_crate_mask")),(char**) NULL, 16);
    mtc->mtcd.ped_crate_mask = strtoul(pt_string_get(pt_map_get(mtcd,"ped_crate_mask")),(char**) NULL, 16);
    mtc->mtcd.control_mask = strtoul(pt_string_get(pt_map_get(mtcd,"control_mask")),(char**) NULL, 16);

    for (i=0;i<6;i++){
	sprintf(mtc->mtca.triggers[i].id,"%s",pt_string_get(pt_array_get(pt_map_get(nhit,"id"),i)));
	mtc->mtca.triggers[i].threshold = pt_integer_get(pt_array_get(pt_map_get(nhit,"threshold"),i));
	mtc->mtca.triggers[i].mv_per_adc = pt_integer_get(pt_array_get(pt_map_get(nhit,"mv_per_adc"),i));
	mtc->mtca.triggers[i].mv_per_hit = pt_integer_get(pt_array_get(pt_map_get(nhit,"mv_per_hit"),i));
	mtc->mtca.triggers[i].dc_offset = pt_integer_get(pt_array_get(pt_map_get(nhit,"dc_offset"),i));
    }
    for (i=0;i<4;i++){
	sprintf(mtc->mtca.triggers[i+6].id,"%s",pt_string_get(pt_array_get(pt_map_get(esum,"id"),i)));
	mtc->mtca.triggers[i+6].threshold = pt_integer_get(pt_array_get(pt_map_get(esum,"threshold"),i));
	mtc->mtca.triggers[i+6].mv_per_adc = pt_integer_get(pt_array_get(pt_map_get(esum,"mv_per_adc"),i));
	mtc->mtca.triggers[i+6].mv_per_hit = pt_integer_get(pt_array_get(pt_map_get(esum,"mv_per_hit"),i));
	mtc->mtca.triggers[i+6].dc_offset = pt_integer_get(pt_array_get(pt_map_get(esum,"dc_offset"),i));
    }
    for (i=0;i<4;i++){
	sprintf(mtc->mtca.triggers[i+10].id,"%s",pt_string_get(pt_array_get(pt_map_get(spare,"id"),i)));
	mtc->mtca.triggers[i+10].threshold = pt_integer_get(pt_array_get(pt_map_get(spare,"threshold"),i));
	mtc->mtca.triggers[i+10].mv_per_adc = pt_integer_get(pt_array_get(pt_map_get(spare,"mv_per_adc"),i));
	mtc->mtca.triggers[i+10].mv_per_hit = pt_integer_get(pt_array_get(pt_map_get(spare,"mv_per_hit"),i));
	mtc->mtca.triggers[i+10].dc_offset = pt_integer_get(pt_array_get(pt_map_get(spare,"dc_offset"),i));
    }
    return 0;
}

int parse_test(pt_node_t* test,mb_t* mb,int cbal,int zdisc,int all)
{
    int j,k;
    if (cbal == 1 || all == 1){
	for (j=0;j<2;j++){
	    for (k=0;k<32;k++){
		mb->vbal[j][k] = pt_integer_get(pt_array_get(pt_array_get(pt_map_get(test,"vbal"),j),k)); 
	    }
	}
    }
    if (zdisc == 1 || all == 1){
	for (k=0;k<32;k++){
	    mb->vthr[k] = pt_integer_get(pt_array_get(pt_map_get(test,"vthr"),k)); 
	}
    }
    return 0;
}

int parse_fec_debug(pt_node_t* value,mb_t* mb)
{
    int j,k;
    for (j=0;j<2;j++){
	for (k=0;k<32;k++){
	    mb->vbal[j][k] = pt_integer_get(pt_array_get(pt_array_get(pt_map_get(value,"vbal"),j),k)); 
	    //printf("[%d,%d] - %d\n",j,k,mb->vbal[j][k]);
	}
    }
    for (k=0;k<32;k++){
	mb->vthr[k] = pt_integer_get(pt_array_get(pt_map_get(value,"vthr"),k)); 
	//printf("[%d] - %d\n",k,mb->vthr[k]);
    }
    for (j=0;j<8;j++){
	mb->tdisc.rmp[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tdisc"),"rmp"),j));	
	mb->tdisc.rmpup[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tdisc"),"rmpup"),j));	
	mb->tdisc.vsi[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tdisc"),"vsi"),j));	
	mb->tdisc.vli[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tdisc"),"vli"),j));	
	//printf("[%d] - %d %d %d %d\n",j,mb->tdisc.rmp[j],mb_consts->tdisc.rmpup[j],mb_consts->tdisc.vsi[j],mb_consts->tdisc.vli[j]);
    }
    mb->tcmos.vmax = pt_integer_get(pt_map_get(pt_map_get(value,"tcmos"),"vmax"));
    mb->tcmos.tacref = pt_integer_get(pt_map_get(pt_map_get(value,"tcmos"),"tacref"));
    //printf("%d %d\n",mb->tcmos.vmax,mb_consts->tcmos.tacref);
    for (j=0;j<2;j++){
	mb->tcmos.isetm[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tcmos"),"isetm"),j));
	mb->tcmos.iseta[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tcmos"),"iseta"),j));
	//printf("[%d] - %d %d\n",j,mb->tcmos.isetm[j],mb_consts->tcmos.iseta[j]);
    }
    for (j=0;j<32;j++){
	mb->tcmos.tac_shift[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tcmos"),"tac_shift"),j));
	//printf("[%d] - %d\n",j,mb->tcmos.tac_shift[j]);
    }
    mb->vint = pt_integer_get(pt_map_get(value,"vint"));
    mb->hvref = pt_integer_get(pt_map_get(value,"hvref"));
    //printf("%d %d\n",mb->vres,mb_consts->hvref);
    for (j=0;j<32;j++){
	mb->tr100.mask[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tr100"),"mask"),j));
	mb->tr100.tdelay[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tr100"),"delay"),j));
	mb->tr20.mask[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tr20"),"mask"),j));
	mb->tr20.tdelay[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tr20"),"delay"),j));
	mb->tr20.twidth[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(value,"tr20"),"width"),j));
	//printf("[%d] - %d %d %d %d %d\n",j,mb->tr100.mask[j],mb_consts->tr100.tdelay[j],mb_consts->tr20.tdelay[j],mb_consts->tr20.twidth[j]);
    }
    for (j=0;j<32;j++){
	mb->scmos[j] = pt_integer_get(pt_array_get(pt_map_get(value,"scmos"),j));
	//printf("[%d] - %d\n",j,mb->scmos[j]);
    }
    mb->disable_mask = 0x0;
    for (j=0;j<32;j++){
	if (pt_integer_get(pt_array_get(pt_map_get(value,"chan_disable"),j)) != 0)
	    mb->disable_mask |= (0x1<<j);
    }

    return 0;

}


int parse_fec_hw(pt_node_t* value,mb_t* mb)
{
    int j,k;
    pt_node_t* hw = pt_map_get(value,"hw");
    mb->mb_id = strtoul(pt_string_get(pt_map_get(value,"board_id")),(char**)NULL,16);
    mb->dc_id[0] = strtoul(pt_string_get(pt_map_get(pt_map_get(hw,"id"),"db0")),(char**)NULL,16);
    mb->dc_id[1] = strtoul(pt_string_get(pt_map_get(pt_map_get(hw,"id"),"db1")),(char**)NULL,16);
    mb->dc_id[2] = strtoul(pt_string_get(pt_map_get(pt_map_get(hw,"id"),"db2")),(char**)NULL,16);
    mb->dc_id[3] = strtoul(pt_string_get(pt_map_get(pt_map_get(hw,"id"),"db3")),(char**)NULL,16);
    //printf("%04x,%04x,%04x,%04x\n",mb->mb_id,mb_consts->dc_id[0],mb_consts->dc_id[1],mb_consts->dc_id[2],mb_consts->dc_id[3]);
    for (j=0;j<2;j++){
	for (k=0;k<32;k++){
	    mb->vbal[j][k] = pt_integer_get(pt_array_get(pt_array_get(pt_map_get(hw,"vbal"),j),k)); 
	    //printf("[%d,%d] - %d\n",j,k,mb->vbal[j][k]);
	}
    }
    for (k=0;k<32;k++){
	mb->vthr[k] = pt_integer_get(pt_array_get(pt_map_get(hw,"vthr"),k)); 
	//printf("[%d] - %d\n",k,mb->vthr[k]);
    }
    for (j=0;j<8;j++){
	mb->tdisc.rmp[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tdisc"),"rmp"),j));	
	mb->tdisc.rmpup[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tdisc"),"rmpup"),j));	
	mb->tdisc.vsi[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tdisc"),"vsi"),j));	
	mb->tdisc.vli[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tdisc"),"vli"),j));	
	//printf("[%d] - %d %d %d %d\n",j,mb->tdisc.rmp[j],mb_consts->tdisc.rmpup[j],mb_consts->tdisc.vsi[j],mb_consts->tdisc.vli[j]);
    }
    mb->tcmos.vmax = pt_integer_get(pt_map_get(pt_map_get(hw,"tcmos"),"vmax"));
    mb->tcmos.tacref = pt_integer_get(pt_map_get(pt_map_get(hw,"tcmos"),"tacref"));
    //printf("%d %d\n",mb->tcmos.vmax,mb_consts->tcmos.tacref);
    for (j=0;j<2;j++){
	mb->tcmos.isetm[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tcmos"),"isetm"),j));
	mb->tcmos.iseta[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tcmos"),"iseta"),j));
	//printf("[%d] - %d %d\n",j,mb->tcmos.isetm[j],mb_consts->tcmos.iseta[j]);
    }
    for (j=0;j<32;j++){
	mb->tcmos.tac_shift[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tcmos"),"tac_shift"),j));
	//printf("[%d] - %d\n",j,mb->tcmos.tac_shift[j]);
    }
    mb->vint = pt_integer_get(pt_map_get(hw,"vint"));
    mb->hvref = pt_integer_get(pt_map_get(hw,"hvref"));
    //printf("%d %d\n",mb->vres,mb_consts->hvref);
    for (j=0;j<32;j++){
	mb->tr100.mask[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tr100"),"mask"),j));
	mb->tr100.tdelay[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tr100"),"delay"),j));
	mb->tr20.mask[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tr20"),"mask"),j));
	mb->tr20.tdelay[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tr20"),"delay"),j));
	mb->tr20.twidth[j] = pt_integer_get(pt_array_get(pt_map_get(pt_map_get(hw,"tr20"),"width"),j));
	//printf("[%d] - %d %d %d %d %d\n",j,mb->tr100.mask[j],mb_consts->tr100.tdelay[j],mb_consts->tr20.tdelay[j],mb_consts->tr20.twidth[j]);
    }
    for (j=0;j<32;j++){
	mb->scmos[j] = pt_integer_get(pt_array_get(pt_map_get(hw,"scmos"),j));
	//printf("[%d] - %d\n",j,mb->scmos[j]);
    }
    mb->disable_mask = 0x0;
    for (j=0;j<32;j++){
	if (pt_integer_get(pt_array_get(pt_map_get(hw,"chan_disable"),j)) != 0)
	    mb->disable_mask |= (0x1<<j);
    }

    return 0;

}

int swap_fec_db(mb_t* mb)
{
    SwapShortBlock(&(mb->mb_id),1);
    SwapShortBlock((mb->dc_id),4);
    SwapShortBlock((mb->scmos),32);
    SwapLongBlock(&(mb->disable_mask),1);
    return 0;
}

int post_debug_doc(int crate, int card, pt_node_t* doc)
{
    char mb_ids[8],dc_ids[4][8],hv_ids[8];
    char rand_id[50];
    char put_db_address[500];
    update_crate_config(crate,0x1<<card);
    get_new_id(rand_id);
    time_t the_time;
    the_time = time(0); //
    char datetime[100];
    sprintf(datetime,"%s",ctime(&the_time));

    sprintf(mb_ids,"%04x",crate_config[crate][card].mb_id);
    sprintf(dc_ids[0],"%04x",crate_config[crate][card].dc_id[0]);
    sprintf(dc_ids[1],"%04x",crate_config[crate][card].dc_id[1]);
    sprintf(dc_ids[2],"%04x",crate_config[crate][card].dc_id[2]);
    sprintf(dc_ids[3],"%04x",crate_config[crate][card].dc_id[3]);
    pt_map_set(doc,"db0_id",pt_string_new(dc_ids[0]));
    pt_map_set(doc,"db1_id",pt_string_new(dc_ids[1]));
    pt_map_set(doc,"db2_id",pt_string_new(dc_ids[2]));
    pt_map_set(doc,"db3_id",pt_string_new(dc_ids[3]));
    pt_map_set(doc,"mb_id",pt_string_new(mb_ids));
    sprintf(hv_ids,"%d",crate);
    pt_map_set(doc,"crate",pt_string_new(hv_ids));
    sprintf(hv_ids,"%d",card);
    pt_map_set(doc,"slot",pt_string_new(hv_ids));
    pt_map_set(doc,"timestamp",pt_integer_new((long int) the_time));
    pt_map_set(doc,"datetime",pt_string_new(datetime));
    pt_map_set(doc,"location",pt_integer_new(current_location));

    sprintf(put_db_address,"http://%s:%s/%s/%s",DB_ADDRESS,DB_PORT,DB_BASE_NAME,rand_id);
    pt_response_t* put_response = pt_put(put_db_address, doc);
    if (put_response->response_code != 201){
	printf("error code %d\n",(int)put_response->response_code);
	pt_free_response(put_response);
	return -1;
    }
    pt_free_response(put_response);
    return 0;
};

int post_debug_doc_with_id(int crate, int card, char *id, pt_node_t* doc)
{
    char mb_ids[8],dc_ids[4][8],hv_ids[8];
    char put_db_address[500];
    update_crate_config(crate,0x1<<card);
    time_t the_time;
    the_time = time(0); //
    char datetime[100];
    sprintf(datetime,"%s",ctime(&the_time));

    sprintf(mb_ids,"%04x",crate_config[crate][card].mb_id);
    sprintf(dc_ids[0],"%04x",crate_config[crate][card].dc_id[0]);
    sprintf(dc_ids[1],"%04x",crate_config[crate][card].dc_id[1]);
    sprintf(dc_ids[2],"%04x",crate_config[crate][card].dc_id[2]);
    sprintf(dc_ids[3],"%04x",crate_config[crate][card].dc_id[3]);
    pt_map_set(doc,"db0_id",pt_string_new(dc_ids[0]));
    pt_map_set(doc,"db1_id",pt_string_new(dc_ids[1]));
    pt_map_set(doc,"db2_id",pt_string_new(dc_ids[2]));
    pt_map_set(doc,"db3_id",pt_string_new(dc_ids[3]));
    pt_map_set(doc,"mb_id",pt_string_new(mb_ids));
    sprintf(hv_ids,"%d",crate);
    pt_map_set(doc,"crate",pt_string_new(hv_ids));
    sprintf(hv_ids,"%d",card);
    pt_map_set(doc,"slot",pt_string_new(hv_ids));
    pt_map_set(doc,"timestamp",pt_integer_new((long int) the_time));
    pt_map_set(doc,"datetime",pt_string_new(datetime));
    pt_map_set(doc,"location",pt_integer_new(current_location));

    sprintf(put_db_address,"http://%s:%s/%s/%s",DB_ADDRESS,DB_PORT,DB_BASE_NAME,id);
    pt_response_t* put_response = pt_put(put_db_address, doc);
    if (put_response->response_code != 201){
	printf("error code %d\n",(int)put_response->response_code);
	pt_free_response(put_response);
	return -1;
    }
    pt_free_response(put_response);
    return 0;
};
