#include "taskCounterBraids.h"


void getHashValue(struct flowTuple *flow, uint32 *index_hash){
	uint8 key[13]; 
	flow2Byte(flow, key);

/*	index_hash[0] = CRC16_1(key, 13);
	index_hash[1] = CRC16_2(key, 13);
	index_hash[3] = CRC16_3(key, 13);*/

	int i =0;
	for(i = 0; i< NUM_HASH; i++){
		index_hash[i] = calculateCRC32(key, 13, crc32Table[i]);
		index_hash[i] = calculateHash32(index_hash[i]);
	}
}

void getHashValue_Layer2(uint32 index_hash, uint32 *index_hash_Layer2){
	int i = 0;
	int32views index;
	index.as_int32 = index_hash; 
	for (i = 0; i < NUM_HASH; i++){
		index_hash_Layer2[i] = calculateCRC32(index.as_int8s, 4, crc32Table[i]);
		index_hash_Layer2[i] = index_hash_Layer2[i]%(NUM_CONTER_2_LAYER);
	}
}



void initialCounterBraids(tHashTable *hashTable, tFlowTable *flowTable){
	int i = 0;
	for(i = 0; i < NUM_CONTER_1_LAYER; i++){
		hashTable[i].count = 0;
		hashTable[i].statusBit = 0;
		hashTable[i].vList = NULL;
	}
	for(i= 0; i< MAX_NUM_FLOW; i++){
		flowTable[i].count = 0;
	}
}

void initialCounterBraids_Layer2(tHashTable *hashTable, tFlowTable *flowTable){
	int i = 0;
	for(i = 0; i < NUM_CONTER_2_LAYER; i++){
		hashTable[i].count = 0;
		hashTable[i].statusBit = 0;
		hashTable[i].vList = NULL;
	}
	for(i= 0; i< NUM_CONTER_1_LAYER; i++){
		flowTable[i].count = 0;
	}
}



void addFlow(tFlowTable *flowTable, int *index_flowTable, struct flowTuple *flow){

	cpyFlowTuple(&(flowTable[*index_flowTable].ft), flow);
	flowTable[*index_flowTable].count = 0;
	uint32 index_hash[NUM_HASH];
	getHashValue(flow, index_hash);

	int i;
	for(i = 0; i< NUM_HASH; i++){
		flowTable[*index_flowTable].uList[i].count_value = 0;
		flowTable[*index_flowTable].index_hash[i] = index_hash[i];
	}

	*index_flowTable += 1;
}

/*
void updateCounterBraids(tHashTable *hashTable, struct flowTuple *flow){
	uint32 index_hash[NUM_HASH];
	getHashValue(flow, index_hash);

	int i;
	for(i = 0; i < NUM_HASH; i++){
		hashTable[index_hash[i]].count += 1;
	}
}
*/
void updateCounterBraids(tHashTable *hashTable, struct flowTuple *flow, tHashTable *hashTable_Layer2, int *index_flowTable_Layer2, tFlowTable *flowTable_Layer2){
	uint32 index_hash[NUM_HASH];
	getHashValue(flow, index_hash);

	int i;
	for(i = 0; i < NUM_HASH; i++){
		hashTable[index_hash[i]].count += 1;
		if(hashTable[index_hash[i]].count == MAX_NUM_1_LAYER){
			hashTable[index_hash[i]].count = 0;
			// getHashValue_Layer2;
			uint32 index_hash_Layer2[NUM_HASH];
			getHashValue_Layer2(index_hash[i], index_hash_Layer2);
			// first overflow;
			if(hashTable[index_hash[i]].statusBit == 0){
				hashTable[index_hash[i]].statusBit = 1;
				//addFlow_Layer2(flowTable_Layer2, &index_flowTable_Layer2, index_hash[i]);
				flowTable_Layer2[*index_flowTable_Layer2].entryPosition = index_hash[i];
				flowTable_Layer2[*index_flowTable_Layer2].count = 0;
				// flowTable_Layer2 initial;
				int perHash =0;
				for(perHash = 0; perHash < NUM_HASH; perHash++){
					flowTable_Layer2[*index_flowTable_Layer2].index_hash[perHash] = index_hash_Layer2[perHash];
					flowTable_Layer2[*index_flowTable_Layer2].uList[perHash].count_value = 0;
				}
				*index_flowTable_Layer2 += 1;
			}
			// update flowTable_Layer2;
			int perHash = 0;
			for(perHash = 0; perHash < NUM_HASH; perHash ++){
				hashTable_Layer2[index_hash_Layer2[perHash]].count +=1;
				//printf("hash_index_layer2:%u\n", index_hash_Layer2[perHash]);
			}
			//printf("------------------------------------\n");
		}
	}
}


void decodeInitial(tHashTable *hashTable, tFlowTable *flowTable, int num_flow, tCounter *hashTableCount){
	int count_index = 0;	// used to allocate hashTable entry;

	// for each flow;
	int perFlow;
	for(perFlow = 0; perFlow < num_flow; perFlow++){
		// for each hash;
		int perHash;
		for(perHash = 0; perHash < NUM_HASH; perHash++){
			int index = flowTable[perFlow].index_hash[perHash];
			flowTable[perFlow].uList[perHash].count_value = hashTable[index].count;
			tCounter *pCount = hashTable[index].vList;
			tCounter *preCount = NULL;
			while(pCount != NULL) {
				//if(index == 5129)
				//	printf("flowID: %d, hashID: %d\n", pCount->flowID, pCount->hashID);
				preCount = pCount;
				pCount = pCount->cNext;
			}

			// malloc 
			tCounter *newCount = &hashTableCount[count_index++];
			newCount->hashID = perHash;
			newCount->flowID = perFlow;
			newCount->count_value = 0;
			newCount->cNext = NULL;

			if(preCount == NULL)
				hashTable[index].vList = newCount;
			else 	
				preCount->cNext = newCount;
		}
	}
}


void decodeCounterBraids(tHashTable *hashTable, tFlowTable *flowTable, int num_flow, tCounter *hashTableCount, int num_entry){
	uint16 index_hash[NUM_HASH];
	//tCounter *htCount;

	uint32 max, min;
	uint32 v_value, u_value, total_value;

	//test
	FILE *fp_f, *fp_h;
	if((fp_f = fopen("flowTable.txt","a"))==NULL){
		printf("open flowTable.txe error\n");
		exit(0);
	}
	fp_h = fopen("hashTable.txt","a");

	decodeInitial(hashTable, flowTable, num_flow, hashTableCount);

	//printFlowTable_decode(fp_f, flowTable, num_flow);
	//printHashTable_decode(fp_h, hashTable, num_entry);

	int perLoop, perFlow, perHash, perHashEntry;

	for(perLoop = 1; perLoop < NUM_ITERATION; perLoop++){
		// flowTable to hashTable
		for(perFlow = 0; perFlow < num_flow; perFlow++){


			// n is the id of the hashFunction;
			for(perHash = 0; perHash < NUM_HASH; perHash++){
				//calculate max or min value that used as the value to hashTable;
				max = 0;
				min = 20000000;
				//Via(t) =  min (b!=a)  Ubi(t) 	if t is odd;
				//	= max (b!=a)  Ubi(t)	if t is even;

				//  calculate Via
				int m;
				for(m= 0; m < NUM_HASH; m++){
					if(m == perHash) continue;
					if(max < flowTable[perFlow].uList[m].count_value)	max = flowTable[perFlow].uList[m].count_value;
					if(min > flowTable[perFlow].uList[m].count_value) min = flowTable[perFlow].uList[m].count_value;	
				}
				if(perLoop%2)	v_value = min;
				else v_value = max;

				// fill the hashTable;
				int index = flowTable[perFlow].index_hash[perHash];
				tCounter *pCount;
				pCount = hashTable[index].vList;
				while(pCount != NULL){
					if((pCount->flowID == perFlow) && (pCount->hashID == perHash))	{
						pCount->count_value = v_value;
						if(perFlow == 18213)
							printf("ljn\n");
						break;
					}
					else pCount = pCount->cNext;
					if(perFlow == 18213){
						printf("%s\n", "ljn-2");
						if(pCount != NULL)
							printf("%d\n", pCount->flowID);
					}
				}
				if(perFlow == 18213)
					printf("3\n");
			}
		}


		// hashTable to flowTable

		// for each hash entry;
		for(perHashEntry = 0; perHashEntry < num_entry; perHashEntry++){
			tCounter *pCount = hashTable[perHashEntry].vList;	
			while(pCount){
				tCounter *ppCount = hashTable[perHashEntry].vList;
				total_value = hashTable[perHashEntry].count;
				while(ppCount){
					if(ppCount != pCount) {
						if(total_value > ppCount->count_value)
							total_value -= ppCount->count_value;
						else{
							total_value = 0;
							break;
						}
					}
					ppCount = ppCount->cNext;
				}

				if(total_value > 1)
					u_value = total_value;
				else
					u_value = 1;
				//printf("u_value:%u\n", u_value);
				//int u_value = getMaxValue( (hashTable[j].count - pCount->count_value), 1);
				flowTable[pCount->flowID].uList[pCount->hashID].count_value = u_value;
				pCount = pCount->cNext;
			}
		}
		//printf("%d-----------iteration\n", perLoop);
		//printFlowTable_decode(fp_f, flowTable, num_flow);
		//printHashTable_decode(fp_h, hashTable, num_entry);

	}
	// update flowTable
	for(perFlow = 0; perFlow < num_flow; perFlow++){

		// n is the id of the hashFunction;
		min = 20000000;
		for(perHash = 0; perHash < NUM_HASH; perHash++){
			if(min > flowTable[perFlow].uList[perHash].count_value) 
				min = flowTable[perFlow].uList[perHash].count_value;
		}
		flowTable[perFlow].count = min;
	}
	//printHashIndex(flowTable, num_flow);
}

void changeFlowTableToHashTable_Layer2(tHashTable *hashTable,tFlowTable *flowTable_Layer2, int num_flow_Layer2){
	int i =0;
	int index_entry;
	for(i = 0; i< num_flow_Layer2; i++){
		index_entry = flowTable_Layer2[i].entryPosition;
		hashTable[index_entry].count += (flowTable_Layer2[i].count * MAX_NUM_1_LAYER);
	}
}


void printFlowStatics(FILE *fp_w, tFlowTable *flowTable, int num_flow){
	int i;
	for(i =0 ;i < num_flow; i++){
		fprintf(fp_w, "%d\t%x\t%x\t%hd\t%hd\t%hhu\t%u\n", i, flowTable[i].ft.src_ip, flowTable[i].ft.dst_ip, flowTable[i].ft.src_port, flowTable[i].ft.dst_port, 
			flowTable[i].ft.proto, flowTable[i].count);
	}
}


void printHashTable(tHashTable *hashTable){
	int i;
	for(i = 0; i< NUM_CONTER_1_LAYER; i++){
		printf("%dth:\t%u\n", i, hashTable[i].count);
	}
	printf("-------------\n");
}


void printFlowTable_decode(FILE* fp, tFlowTable *flowTable, int num_flow){
	fprintf(fp, "-----flowTable------%d\n",num_flow);	
	int i;
	for(i = 0; i< num_flow; i++){
		fprintf(fp, "%dth:\t%u\t1:\t%u\n",i, flowTable[i].uList[0].count_value, flowTable[i].uList[1].count_value);
	}
}

void printHashTable_decode(FILE *fp, tHashTable *hashTable, int num_entry){
	fprintf(fp, "------hashTable-------%d\n",num_entry);
	int i;
	for(i = 0; i< num_entry; i++){
		fprintf(fp, "%dth:\t", i);
		int j = 0;
		tCounter *pCount = hashTable[i].vList;
		while(pCount){
			fprintf(fp, "%d:\t%u\t", j++, pCount->count_value);
			pCount = pCount->cNext;
		}
		fprintf(fp, "\n");
	}
}

void printHashIndex(tFlowTable *flowTable, int num_flow){
	FILE *fp;
	fp = fopen("hashIndex.txt","w");
	int i;
	for(i = 0; i< num_flow; i++){
		fprintf(fp, "%d\t1:%u\t2:%u\n", i, flowTable[i].index_hash[0], flowTable[i].index_hash[1]);
	}
}