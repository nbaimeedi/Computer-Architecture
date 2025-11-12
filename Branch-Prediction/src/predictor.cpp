//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Baimeedi Nithin Reddy";
const char *studentID = "A69029730";
const char *email = "nbaimeedi.ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
//
// define number of bits required for indexing the BHT here.
int ghistoryBits = 17; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;


int tournament_g_ghistorybits = 16;
uint8_t *tournament_g_bht;
uint64_t tournament_g_ghistory;

int tournament_l_lhistory_bits = 14;
int tournament_pc_bits = 13;
uint8_t *tournament_l_bht;
uint8_t *tournament_l_lht;
uint64_t tournament_l_lhistory;

int tournament_chooser_ghistorybits = 16;
int8_t *tournament_chooser;
uint64_t tournament_chooser_ghistory;


#include <stdio.h>
#include <string.h>
#include <math.h>


#define PERCEPTRONS 427
#define GHISTORY 59

uint32_t hash_pc(uint32_t pc)
{
  uint64_t temp = pc * GHISTORY;
  return (temp % PERCEPTRONS);
}

int16_t perceptron_weight[PERCEPTRONS][GHISTORY + 1];
int16_t perceptron_ghistory[GHISTORY];
int32_t perceptron_threshold;
uint32_t perceptron_prediction = NOTTAKEN;
uint8_t threshold_fail = 0;


void perceptron_saturation_counter(int16_t* counter_bits, uint8_t match){
  if(match)
  {
    if(*counter_bits != 255)
    {
      (*counter_bits)++;
    }
  }else
  {
    if(*counter_bits != -256)
    {
      (*counter_bits)--;
    }
  }
}

void perceptron_init(){
  perceptron_threshold = (int32_t)(2 * GHISTORY + 15);
  //printf("threshold: %d\n",perceptron_threshold);
  memset(perceptron_weight, 0, sizeof(int16_t) * PERCEPTRONS * (GHISTORY + 1));
  memset(perceptron_ghistory, 0, sizeof(uint16_t) * GHISTORY);
}



uint32_t perceptron_predict(uint32_t pc){
  uint32_t index = hash_pc(pc);
  //printf("hashed pc: %d\n",index);
  int16_t perceptron_function = perceptron_weight[index][0];
  //printf("perceptron_function: %d",perceptron_function);

  for(int i = 1 ; i <= GHISTORY ; i++)
  {
    if (perceptron_ghistory[i-1])
    {
      perceptron_function += perceptron_weight[index][i];
    }
    else
    {
      perceptron_function += -perceptron_weight[index][i];
    }
  }
  //printf("perceptron_function: %d",perceptron_function);
  perceptron_prediction = (perceptron_function >= 0) ? TAKEN : NOTTAKEN;
  //printf("prediction: %d\n",perceptron_prediction);
  if (perceptron_function < perceptron_threshold && perceptron_function > -perceptron_threshold)
  {
    threshold_fail = 1;
  }
  else
  {
    threshold_fail = 0;
  }
  //printf("threshold_fail: %d",threshold_fail);
  return perceptron_prediction;
}

void perceptron_train(uint32_t pc, uint32_t outcome){

  uint32_t index = hash_pc(pc);
  //printf("hashed pc: %d\n",index);

  if((perceptron_prediction != outcome) || threshold_fail)
  {
    perceptron_saturation_counter(&(perceptron_weight[index][0]), outcome);
    for(int i = 1 ; i <= GHISTORY ; i++){
      uint32_t predict = perceptron_ghistory[i-1];
      //printf("predict:%d\n",predict);
      perceptron_saturation_counter(&(perceptron_weight[index][i]), (outcome == predict));
    }

  }

  for(int i = GHISTORY - 1; i > 0 ; i--)
  {
    perceptron_ghistory[i] = perceptron_ghistory[i-1];
  }
  perceptron_ghistory[0] = outcome;
  //printf("perceptron ghistory: %d\n",perceptron_ghistory);

}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    //printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    //printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}

void init_tournament()
{
  //printf("Entered Initialization Stage\n");
  //initialize the global predictor of the tournament predictor

  //get the number of bht entries
  int tournament_g_bht_entries = 1 << tournament_g_ghistorybits;
  ////printf("%d",tournament_g_bht_entries);
  //assign memory for bht and use tournament_g_bht to point to the first address of bht
  tournament_g_bht = (uint8_t *)malloc(tournament_g_bht_entries * sizeof(uint8_t));
  //initialize each entry in bht to weakly not taken
  int i = 0;
  for(i = 0; i < tournament_g_bht_entries; i++)
  {
    tournament_g_bht[i] = WT;
  }
  //initialize ghr
  tournament_g_ghistory = 0;
  //printf("Tournament Global Predictor Initialization Complete\n");

  //initialize the local predictor of the tournament predictor

  //get the number of bht entries
  int tournament_l_bht_entries = 1 << tournament_l_lhistory_bits;
  //assign memory to bht and use tournament_l_bht to point to the first address of bht
  tournament_l_bht = (uint8_t *)malloc(tournament_l_bht_entries * sizeof(uint8_t));
  //get the number of lht entries 
  int tournament_l_lht_entries = 1 << tournament_pc_bits;
  //assign memory to lht and use tournament_l_lht to point to the first address of lht;
  tournament_l_lht = (uint8_t *)malloc(tournament_l_lht_entries * sizeof(uint8_t));
  //initialize the bht entries with weakly not taken
  i = 0;
  for(i = 0; i < tournament_l_bht_entries; i++)
  {
    tournament_l_bht[i] = WN;
  }
  //initialize lht entries
  int j = 0;
  for(j = 0; j < tournament_l_lht_entries; j ++)
  {
    tournament_l_lht[j] = 0;
  }

  ////printf("%d",tournament_chooser_ghistorybits);
  //printf("Tournament Local Predictor Initialization Complete\n");
  //Initialize chooser of the touranment predictor
  int tournament_chooser_entries = 1 << tournament_chooser_ghistorybits;
  ////printf("%d",tournament_chooser_ghistorybits);
  tournament_chooser = (int8_t *)malloc(tournament_chooser_entries * sizeof(int8_t));
  //initialize the chooser entries (take the global predictors prediction)
  int k = 0;
  for(k = 0; k < (tournament_chooser_entries); k++)
  {
    tournament_chooser[k] = 1;
  }
  //for(k = (tournament_chooser_entries/2); k < tournament_chooser_entries; k++)
  //{
    //tournament_chooser[k] = 0;
  //}
  //initialize chooser's ghr
  tournament_chooser_ghistory = 0;
  //printf("Tournament Chooser Initialization Complete\n");

  //printf("Tournament G BHT[0]: %d\n",tournament_g_bht[0]);
  //printf("Tournament L BHT[0]: %d\n",tournament_l_bht[0]);
  //printf("Tournament CHOOSER[0]: %d\n",tournament_chooser[0]);

  //printf("Tournament G BHT[END]: %d\n",tournament_g_bht[tournament_g_bht_entries-1]);
  //printf("Tournament L BHT[END]: %d\n",tournament_l_bht[tournament_l_bht_entries-1]);
  //printf("Tournament CHOOSER[END]: %d\n",tournament_chooser[tournament_chooser_entries-1]);


}

uint32_t tournament_predict(uint32_t pc)
{
    //printf("Entered Prediction Stage\n");
    uint32_t tournament_g_prediction;
    uint32_t tournament_l_prediction;

    //printf("Tournament G GHISTORY BITS: %d\n",tournament_g_ghistorybits);
    uint32_t tournament_g_bht_entries = 1 << tournament_g_ghistorybits;
    //printf("Tournament G BHT ENTRIES: %d\n",tournament_g_bht_entries);
    uint32_t tournament_g_ghistory_lower_bits = tournament_g_ghistory & (tournament_g_bht_entries - 1);
    uint32_t tournament_g_pc_lower_bits = pc & (tournament_g_bht_entries - 1);

    uint32_t tournament_l_bht_entries = 1 << tournament_l_lhistory_bits;
    //printf("TOURNAMENT L BHT ENTRIES: %d\n",tournament_l_bht_entries);
    uint32_t tournament_l_lht_entries = 1 << tournament_pc_bits;
    //printf("TOURNAMENT L LHT ENTRIES: %d\n",tournament_l_lht_entries);

    uint32_t tournament_chooser_entries = 1 << tournament_chooser_ghistorybits;
    uint32_t tournament_chooser_ghistory_lower_bits = tournament_chooser_ghistory & (tournament_chooser_entries - 1);
    uint32_t tournament_chooser_pc_lower_bits = pc & (tournament_chooser_entries - 1);
    //printf("TOURNAMENT CHOOSER ENTRIES: %d\n",tournament_chooser_entries);

    uint32_t pc_lower_bits = pc & (tournament_l_lht_entries - 1);
    //printf("PC: %d\n",pc);
    //printf("PC LOWER BITS: %d\n",pc_lower_bits);

    //global predict
    int index = tournament_g_ghistory_lower_bits ^ tournament_g_pc_lower_bits;
    //printf("GLOBAL INDEX: %d\n",index);
    //printf("G BHT[INDEX]: %d\n",tournament_g_bht[index]);
    switch(tournament_g_bht[index])
    {
      case 1:
	//printf("G BHT: WN\n");
        tournament_g_prediction = NOTTAKEN;
	break;
      case 0:
	//printf("G BHT: SN\n");
        tournament_g_prediction = NOTTAKEN;
	break;
      case 2:
	//printf("G BHT: WT\n");
        tournament_g_prediction = TAKEN;
	break;
      case 3:
	//printf("G BHT: ST\n");
        tournament_g_prediction = TAKEN;
	break;
      default:
        //printf("Warning: Undefined state of entry in TOURNAMENT's GLOBAL BHT!\n");
	//printf("DEFAULT: G BHT: SN\n");
        tournament_g_prediction = NOTTAKEN;
    }

    //Local Predict
    index = tournament_l_lht[pc_lower_bits];
    index = index & (tournament_l_bht_entries - 1);
    //printf("LOCAL INDEX: %d\n",index);
    //printf("L BHT[INDEX]: %d\n",tournament_l_bht[pc_lower_bits]);
    switch(tournament_l_bht[index])
    {
      case 1:
	//printf("L BHT: WN\n");
        tournament_l_prediction = NOTTAKEN;
	break;
      case 0:
	//printf("L BHT: SN\n");
        tournament_l_prediction = NOTTAKEN;
	break;
      case 2:
	//printf("L BHT: WT\n");
        tournament_l_prediction = TAKEN;
	break;
      case 3:
	//printf("L BHT: ST\n");
        tournament_l_prediction = TAKEN;
	break;
      default:
        //printf("Warning: Undefined state of entry in TOURNAMENT's LOCAL BHT!\n");
	//printf("G BHT: SN\n");
        tournament_l_prediction = NOTTAKEN;
    }

    //Final Prediction based on tournament's chooser output
    index = tournament_chooser_ghistory_lower_bits;// ^ tournament_chooser_pc_lower_bits;
    //printf("CHOOSER INDEX: %d\n",index);
    //printf("CHOOSER[INDEX]: %d\n",tournament_chooser[index]);
    switch(tournament_chooser[index])
    {
      case 0:
	//printf("CHOOSE GLOBAL PREDICTION\n");
        return tournament_g_prediction;
	break;
      case 1:
	//printf("CHOOSE LOCAL PREDICTION\n");
        return tournament_l_prediction;
	break;
      case -1:
	//printf("CHOOSE GLOBAL PREDICTION\n");
        return tournament_g_prediction;
	break;
      default:
	//printf("CHOOSE LOCAL PREDICTION\n");
        return tournament_l_prediction;
    }
}

void train_tournament(uint32_t pc, uint8_t outcome)
{

  //printf("Entered Training Stage\n");

  uint32_t tournament_chooser_entries = 1 << tournament_chooser_ghistorybits;
  uint32_t tournament_chooser_ghistory_lower_bits = tournament_chooser_ghistory & (tournament_chooser_entries - 1);
  uint32_t tournament_chooser_pc_lower_bits = pc & (tournament_chooser_entries - 1);
  //printf("CHOOSER ENTRIES: %d\n",tournament_chooser_entries);

  uint32_t tournament_g_bht_entries = 1 << tournament_g_ghistorybits;
  uint32_t tournament_g_ghistory_lower_bits = tournament_g_ghistory & (tournament_g_bht_entries - 1);
  uint32_t tournament_g_pc_lower_bits = pc & (tournament_g_bht_entries - 1);
  //printf("G BHT ENTRIES: %d\n",tournament_g_bht_entries);
  //printf("G GHISTORY: %d\n",tournament_g_ghistorybits);
  //printf("G GHISTORY LOWER BITS: %d\n",tournament_g_ghistory_lower_bits);


  uint32_t tournament_l_lht_entries = 1 << tournament_pc_bits;
  uint32_t tournament_l_bht_entries = 1 << tournament_l_lhistory_bits;
  uint32_t pc_lower_bits = pc & (tournament_l_lht_entries - 1);
  //printf("L BHT ENTRIES: %d\n",tournament_l_bht_entries);
  //printf("L LHT ENTRIES: %d\n",tournament_l_lht_entries);
  //printf("PC LOWER BITS: %d\n",pc_lower_bits);

  //update tournament's chooser table entry corresponding to the current branch pc based on outcome
  int tournament_l_prediction;
  int tournament_g_prediction;

  int tournament_l_prediction_valid;
  int tournament_g_prediction_valid;

  int outcome_bit;

  int tournament_chooser_selection;

  int index = tournament_g_ghistory_lower_bits ^ tournament_g_pc_lower_bits;
  tournament_g_prediction = (tournament_g_bht[index] == 1)? 0 : 1;
  tournament_g_prediction = (tournament_g_bht[index] == 0)? 0 : 1;
  tournament_g_prediction = (tournament_g_bht[index] == 2)? 1 : 0;
  tournament_g_prediction = (tournament_g_bht[index] == 3)? 1 : 0;
  //printf("G PREDICTION: %d\n",tournament_g_prediction);

  index = tournament_l_lht[pc_lower_bits];
  index = index & (tournament_l_bht_entries - 1);
  tournament_l_prediction = (tournament_l_bht[index] == 1)? 0 : 1;
  tournament_l_prediction = (tournament_l_bht[index] == 0)? 0 : 1;
  tournament_l_prediction = (tournament_l_bht[index] == 2)? 1 : 0;
  tournament_l_prediction = (tournament_l_bht[index] == 3)? 1 : 0;
  //printf("L PREDICTION: %d\n",tournament_l_prediction);

  outcome_bit = (outcome == TAKEN)? 1 : 0;
  //printf("OUTCOME BIT: %d\n",outcome_bit);

  tournament_l_prediction_valid = (tournament_l_prediction == outcome_bit)? 1 : 0;
  tournament_g_prediction_valid = (tournament_g_prediction == outcome_bit)? 1 : 0;
  //printf("L PREDICTION VALIDATION: %d\n",tournament_l_prediction_valid);
  //printf("G PREDICTION VALIDATION: %d\n",tournament_g_prediction_valid);

  tournament_chooser_selection = tournament_l_prediction_valid - tournament_g_prediction_valid;
  //printf("CHOOSER SELECTION: %d\n",tournament_chooser_selection);
  //index = tournament_chooser_ghistory & (tournament_chooser_entries - 1);
  index = tournament_chooser_ghistory_lower_bits;// ^ tournament_chooser_pc_lower_bits;
  //printf("CHOOSER INDEX: %d\n",index);
  tournament_chooser[index] = tournament_chooser_selection;

  tournament_chooser_ghistory = ((tournament_chooser_ghistory << 1) | (outcome));
  //printf("CHOOSE GHISTORY UPDATED: %d\n",tournament_chooser_ghistory);

  //train tournament's global predictor
  index = tournament_g_ghistory_lower_bits ^ tournament_g_pc_lower_bits;

  //update tournament's global predictor's bht based on outcome
  switch(tournament_g_bht[index])
  {
    case 1:
      tournament_g_bht[index] = (outcome == TAKEN)? WT: SN;
      break;
    case 0:
      tournament_g_bht[index] = (outcome == TAKEN)? WN: SN;
      break;
    case 2:
      tournament_g_bht[index] = (outcome == TAKEN)? ST: WN;
      break;
    case 3:
      tournament_g_bht[index] = (outcome == TAKEN)? ST: WT;
      break;
    default:
      //printf("Warning: Undefined state of entry in TOURNAMENT's GLOBAL BHT!\n");
      break;
  }
  //printf("G BHT[INDEX] UPDATED: %d\n",tournament_g_bht[index]);

  //update tournament's global's global history register
  tournament_g_ghistory = ((tournament_g_ghistory << 1) | outcome);
  //printf("G GHISTORY UPDATED: %d\n",tournament_g_ghistory);


  //train tournament's local predictor

  //update tournament's local predictor's bht based on outcome
  index = tournament_l_lht[pc_lower_bits];
  index = index & (tournament_l_bht_entries - 1);
  switch(tournament_l_bht[index])
  {
    case 1:
      tournament_l_bht[index] = (outcome == TAKEN)? WT: SN;
      break;
    case 0:
      tournament_l_bht[index] = (outcome == TAKEN)? WN: SN;
      break;
    case 2:
      tournament_l_bht[index] = (outcome == TAKEN)? ST: WN;
      break;
    case 3:
      tournament_l_bht[index] = (outcome == TAKEN)? ST: WT;
      break;
    default:
      //printf("Warning: Undefined state of entry in TOURNAMENT's LOCAL BHT!\n");
      break;
  }
  //printf("L BHT[INDEX] UPDATED: %d\n",tournament_l_bht[index]);
  //update tournament's local's local history table's entry corresponding to the current branch pc
  tournament_l_lht[pc_lower_bits] = ((tournament_l_lht[pc_lower_bits] << 1) | outcome);
  //printf("L LHT UPDATED: %d\n",tournament_l_lht[pc_lower_bits]);

  //printf("Tournament G BHT[0]: %d\n",tournament_g_bht[0]);
  //printf("Tournament L BHT[0]: %d\n",tournament_l_bht[0]);
  //printf("Tournament CHOOSER[0]: %d\n",tournament_chooser[0]);
  //printf("Tournament G GHISTORY: %d\n",(tournament_g_ghistory & (tournament_g_bht_entries - 1)));
  //printf("Tournament CHOOSER GHISTORY: %d\n",(tournament_chooser_ghistory & (tournament_chooser_entries - 1)));
  //printf("Tournament L LHT[0]: %d\n",tournament_l_lht[0]);


  //printf("Tournament G BHT[END]: %d\n",tournament_g_bht[tournament_g_bht_entries-1]);
  //printf("Tournament L BHT[END]: %d\n",tournament_l_bht[tournament_l_bht_entries-1]);
  //printf("Tournament CHOOSER[END]: %d\n",tournament_chooser[tournament_chooser_entries-1]);
  //printf("Tournament L LHT[END]: %d\n",tournament_l_lht[tournament_l_lht_entries-1]);

}
// Initialize the predictor
//
void
init_predictor() {
    printf("%d",bpType);
    //
    //TODO: Initialize Branch Predictor Data Structures
    switch (bpType) {
        case STATIC:
            return;
        case TOURNAMENT:
            init_tournament();
            break;

        case GSHARE:
	    init_gshare();
            break;
        case CUSTOM:

            perceptron_init();


        default:
            break;
    }


}


// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t
make_prediction(uint32_t pc, uint32_t target, uint32_t direct) {
    //
    //TODO: Implement prediction scheme
    //
    if (bpType == STATIC) {
        return TAKEN;

    } else if (bpType == GSHARE) {
	    return gshare_predict(pc);

    } else if (bpType == TOURNAMENT) {
	    return tournament_predict(pc);

    } else if (bpType == CUSTOM) {

        return perceptron_predict(pc);

    } else {
        return NOTTAKEN;
    }

}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct) {
    //
    //TODO: Implement Predictor training
    //

    switch (bpType) {
        case STATIC:
            break;
        case TOURNAMENT:
	    train_tournament(pc,outcome);
            break;
        case GSHARE:
	    train_gshare(pc, outcome);
            break;
        case CUSTOM:
            perceptron_train(pc, outcome);
        default:
            break;
    }

    // If there is not a compatable bpType then return NOTTAKEN
    return;
}
