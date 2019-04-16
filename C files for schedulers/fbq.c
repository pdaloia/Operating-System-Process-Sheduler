/*

# Family Name: D'Aloia

# Given Name: Philip

# Section: E

# Student Number: 213672431

# CSE Login: pdaloia

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "sch-helpers.h"

process processes[MAX_PROCESSES + 1];          // a large structure array to hold all processes read from data file 
int numberOfProcesses = 0;                     // total number of processes 
process_queue readyQueue0;  				   // a FIFO structure for the ready queue
process_queue readyQueue1;  				   // a FIFO structure for the ready queue
process_queue readyQueue2;  				   // a FIFO structure for the ready queue
process *processors[NUMBER_OF_PROCESSORS];     // an array of pointers for each of the four processors
process *waitingProcesses[MAX_PROCESSES];      // an array of pointers for processes that are doing an IO burst
int numberOfFinishedProcesses = 0;			   // counter for the number of processes which have finished
process nullProcess; 						   //a process to show an empty spot in a CPU
process_node *temp;							   // a pointer to a temporarily held process, used for traversing ready queue 
float processorUseCounter = 0;				   // counter for how many processors are in use (total)
int firstTimeQuantumSpecified;				   //first time quantum for queue0
int secondTimeQuantumSpecified;				   //second time quantum for queue1
int contextSwitchCounter = 0;				   //count context switches
int currentProcessorsInUse = 0;

//comparing function to order processes by lowest to highest pid if they have the same scheduling criterion
int compareByPid(const void *aa, const void *bb) {
    process *a = *(process**) aa;
    process *b = *(process**) bb;
    if (a->pid < b->pid) return -1;
    if (a->pid > b->pid) return 1;
    return 0;
}

//function to put a process at front of ready queue (for when a process is preempted by a process in a higher queue)
void putProcessAtHead(process_queue *q, process *p) {
    process_node *node = createProcessNode(p);

    if (q->front == NULL) {
        assert(q->back == NULL);
        q->front = q->back = node;
    } else {
        assert(q->front != NULL);
        node->next = q->front;
        q->front = node;
        //q->back->next = node;
        //q->back = node;
    }
    q->size++;
}

int main (int argc, char *argv[]) {
	
	if(argc != 3){
		printf("USAGE: fbq QUANTUM1 QUANTUM2 < CPU-LOAD-FILE\n");
		printf("  eg. \"fbq 10 30 < load\" for a 3-level feedback queue wherein\n");
		printf("      Q1 has time quantum 10, Q2 has quantum 30, and Q3 is FCFS,\n");
		printf("      processing workload in file `load'\n");
		return 0;
	}

	firstTimeQuantumSpecified = atoi(argv[1]);
	secondTimeQuantumSpecified = atoi(argv[2]);
	
	if(firstTimeQuantumSpecified < 1 || secondTimeQuantumSpecified < 1){
		printf("Invalid time quantum: expected integer in range [1,2^31-1].\n");
		return 0;
	}

	//initialize the null process in which array spots considered empty will point to
	nullProcess.pid = -1;

	//counters and status variables used in loops
	int status = 0;
	int i;
	int secondCounter;

	//initialize processors to have the null process (process with pid -1 to know that the processor is free for use)
	for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
		processors[i] = &nullProcess;
	}

	//initialize waiting processes array to contain null process in all spots (free spot to put waiting process in)
	for(i = 0; i < MAX_PROCESSES; i++){
		waitingProcesses[i] = &nullProcess;
	}

	//read in and sort the processes
	while ((status=readProcess(&processes[numberOfProcesses])))  {
         if(status==1)  numberOfProcesses ++;
	}   // it reads pid, arrival_time, bursts to one element of the above struct array
	qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);

	//array used to sort processes if scheduling criterion the same
	process *processesToSort0[numberOfProcesses];
	process *processesToSort1[numberOfProcesses];
	process *processesToSort2[numberOfProcesses];
	for(i = 0; i < numberOfProcesses; i++){
		processesToSort0[i] = &nullProcess;
	}
	for(i = 0; i < numberOfProcesses; i++){
		processesToSort1[i] = &nullProcess;
	}
	for(i = 0; i < numberOfProcesses; i++){
		processesToSort2[i] = &nullProcess;
	}

	initializeProcessQueue(&readyQueue0); //initialize ready queue
	initializeProcessQueue(&readyQueue1); //initialize ready queue
	initializeProcessQueue(&readyQueue2); //initialize ready queue
	int schedulerTime = 0; //start the timer of the scheduler at 0

	//start of while loop that will loop until all processes are finished
	while(1){

		//go through processes and check if any new processes have arrived
		//then add to processesToSort0 so they will be sorted and then added to readyQueue0 from lowest to highest pid
		for(i = 0; i < numberOfProcesses; i++){
			if(processes[i].arrivalTime == schedulerTime){

				for(secondCounter = 0; secondCounter < numberOfProcesses; secondCounter++){
					if(processesToSort0[secondCounter]->pid == -1){
						processesToSort0[secondCounter] = &processes[i];
						processesToSort0[secondCounter]->quantumRemaining = firstTimeQuantumSpecified;
						break;
					} //end of if
				} //end of inner for loop

			} //end of if
		} //end of outter for loop

		//preempt certain processes if all processors are in use
		//if readyQueue0 has process, preempt any processes from queue1 or queue2 that are in a CPU
		//if readyQueue1 has process and readyQueue0 has no processes, preempt any process from queue2 that are in CPU
		currentProcessorsInUse = 0;
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
			if(processors[i]->pid != -1){
				currentProcessorsInUse++;
			} //end of if
		} //end of for loop
		if(currentProcessorsInUse == 4){
			for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
				if(processors[i]->pid != -1){

					if(processors[i]->currentQueue == 1){
						if(readyQueue0.size > 0){
							putProcessAtHead(&readyQueue1, processors[i]);
							processors[i] = readyQueue0.front->data;
							contextSwitchCounter++;
							dequeueProcess(&readyQueue0);
						} //end of if
					} //end of if
					else if (processors[i]->currentQueue == 2){
						if(readyQueue0.size > 0){
							putProcessAtHead(&readyQueue2, processors[i]);
							processors[i] = readyQueue0.front->data;
							contextSwitchCounter++;
							dequeueProcess(&readyQueue0);
						} //end of if
						else if(readyQueue1.size > 0){
							putProcessAtHead(&readyQueue2, processors[i]);
							processors[i] = readyQueue1.front->data;
							contextSwitchCounter++;
							dequeueProcess(&readyQueue1);
						} //end of else if
					} //end of else if

				} //end of if
			} //end of for loop
		} //end of if

		//check if any preocessors are free, if any are free add process from readyQueue0.
		//If readyQueue0 empty, add from readyQueue1.
		//If readyQueue1 empty, add from readyQueue2.
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
			if(processors[i]->pid == -1){

				if(readyQueue0.size > 0){
					processors[i] = readyQueue0.front->data;
					if(processors[i]->quantumRemaining == firstTimeQuantumSpecified && processors[i]->currentBurst == 0){
						processors[i]->startTime = schedulerTime;
					}
					else if(processors[i]->quantumRemaining == 0){
						processors[i]->quantumRemaining = firstTimeQuantumSpecified; 
					}
					/*if(processors[i]->quantumRemaining == 0){
						processors[i]->quantumRemaining = firstTimeQuantumSpecified; 
						if(processors[i]->currentBurst == 0){
							processors[i]->startTime = schedulerTime;
						}
					}*/
					dequeueProcess(&readyQueue0);
				} //end of if
				else if(readyQueue1.size > 0){
					processors[i] = readyQueue1.front->data;
					if(processors[i]->quantumRemaining == 0){
						processors[i]->quantumRemaining = secondTimeQuantumSpecified;
					}
					dequeueProcess(&readyQueue1);
				} //end of else if
				else if(readyQueue2.size > 0){
					processors[i] = readyQueue2.front->data;
					dequeueProcess(&readyQueue2);
				} //end of else

			} //end of if
		} //end of for loop

		//increment all processes step count which are in CPUs
		//decrement quantum remaining
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){

			if(processors[i]->pid != -1){
				processors[i]->bursts[processors[i]->currentBurst].step++;
				processors[i]->quantumRemaining--; //added in rr.c
			} //end of if

		} //end of for loop

		//increment io burst step counter of everything in waiting queue
		for(i = 0; i < MAX_PROCESSES; i++){

			if(waitingProcesses[i]->pid != -1){
				waitingProcesses[i]->bursts[waitingProcesses[i]->currentBurst].step++;
			} //end of if

		} //end of for loop

		//increment wait time of all processes in all ready queues
		if(readyQueue0.size > 0){
			temp = readyQueue0.front;
			while(temp != NULL){
				temp->data->waitingTime++;
				temp = temp->next;
			} //end of while loop
		} //end of if
		if(readyQueue1.size > 0){
			temp = readyQueue1.front;
			while(temp != NULL){
				temp->data->waitingTime++;
				temp = temp->next;
			} //end of while loop
		} //end of if
		if(readyQueue2.size > 0){
			temp = readyQueue2.front;
			while(temp != NULL){
				temp->data->waitingTime++;
				temp = temp->next;
			} //end of while loop
		} //end of if

		//check if any waiting processes done io burst, if they are send to processesToSort0 queue to be sorted before being put into readyQueue0
		for(i = 0; i < MAX_PROCESSES; i++){

			if(waitingProcesses[i]->pid != -1){
				if(waitingProcesses[i]->bursts[waitingProcesses[i]->currentBurst].step == waitingProcesses[i]->bursts[waitingProcesses[i]->currentBurst].length){

					if(waitingProcesses[i]->currentBurst == waitingProcesses[i]->numberOfBursts){
						waitingProcesses[i]->endTime = schedulerTime;
						numberOfFinishedProcesses++;
					} //end of if
					else{
						waitingProcesses[i]->currentBurst++;
						//enqueueProcess(&readyQueue, waitingProcesses[i]);
						for(secondCounter = 0; secondCounter < numberOfProcesses; secondCounter++){
							if(processesToSort0[secondCounter]->pid == -1){
								processesToSort0[secondCounter] = waitingProcesses[i];
								break;
							} //end of if
						} //end of inner for loop
					} //end of else

					waitingProcesses[i] = &nullProcess;
				} //end of if
			} //end of if

		} //end of outter for loop

		//check how many processors are in use at this time of the scheduler
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
			if(processors[i]->pid != -1){
				processorUseCounter++;
			} //end of if
		} //end of for loop
		
		//send processes that are done in CPU to the waiting queue
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
			if(processors[i]->pid != -1){

				if(processors[i]->bursts[processors[i]->currentBurst].step == processors[i]->bursts[processors[i]->currentBurst].length){
					processors[i]->currentBurst++;
					if(processors[i]->currentBurst == processors[i]->numberOfBursts){
						numberOfFinishedProcesses++;
						processors[i]->endTime = schedulerTime;
						processors[i] = &nullProcess;
					} //end of if
					else{
						for(secondCounter = 0; secondCounter < MAX_PROCESSES; secondCounter++){
							if(waitingProcesses[secondCounter]->pid == -1){
								waitingProcesses[secondCounter] = processors[i];
								waitingProcesses[secondCounter]->currentQueue = 0;
								waitingProcesses[secondCounter]->quantumRemaining = 0;
								//waitingProcesses[j]->currentBurst++;
								processors[i] = &nullProcess;
								break;
							} //end of if
						} //end of inner for loop
					} //end of else
				} //end of if

			} //end of if
		} //end of outter for loop

		//check if any processes have been in a processor for the time quantum specified
		//if they are in queue0 (RR1), send to processesToSort1 to be added to readyQueue1 once sorted
		//if they are in queue1 (RR2), send to processesToSort2 to be added to readyQueue2 once sorted
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
			if(processors[i]->pid != -1){

				if(processors[i]->quantumRemaining == 0 && processors[i]->currentQueue == 0){
					for(secondCounter = 0; secondCounter < numberOfProcesses; secondCounter++){
						if(processesToSort1[secondCounter]->pid == -1){
							processesToSort1[secondCounter] = processors[i];
							processesToSort1[secondCounter]->currentQueue++;
							processors[i] = &nullProcess;
							contextSwitchCounter++;
							break;
						}//end of if
					}//end of inner for loop
				} //end of if

				else if(processors[i]->quantumRemaining == 0 && processors[i]->currentQueue == 1){
					for(secondCounter = 0; secondCounter < numberOfProcesses; secondCounter++){
						if(processesToSort2[secondCounter]->pid == -1){
							processesToSort2[secondCounter] = processors[i];
							processesToSort2[secondCounter]->currentQueue++;
							processors[i] = &nullProcess;
							contextSwitchCounter++;
							break;
						}//end of if
					}//end of inner for loop
				} //end of if

			} //end of if
		} //end of outter for loop

		//sort array processesToSort from lowest pid to highest pid
		qsort(processesToSort0, numberOfProcesses, sizeof(process *), &compareByPid);
		qsort(processesToSort1, numberOfProcesses, sizeof(process *), &compareByPid);
		qsort(processesToSort2, numberOfProcesses, sizeof(process *), &compareByPid);

		//enqeue all processes in processesToSort0, processesToSort1, and processesToSort2 into their respective ready queues now that they have been sorted
		for(i = 0; i < numberOfProcesses; i++){
			if(processesToSort0[i]->pid != -1){
				enqueueProcess(&readyQueue0, processesToSort0[i]);
			}
		}
		for(i = 0; i < numberOfProcesses; i++){
			if(processesToSort1[i]->pid != -1){
				enqueueProcess(&readyQueue1, processesToSort1[i]);
			}
		}
		for(i = 0; i < numberOfProcesses; i++){
			if(processesToSort2[i]->pid != -1){
				enqueueProcess(&readyQueue2, processesToSort2[i]);
			}
		}

		//make all spots in processesToSort0, processesToSort1, and processesToSort2 point to the nullProcess since all processes have been put into the ready queue
		for(i = 0; i < numberOfProcesses; i++){
			processesToSort0[i] = &nullProcess;
		}
		for(i = 0; i < numberOfProcesses; i++){
			processesToSort1[i] = &nullProcess;
		}
		for(i = 0; i < numberOfProcesses; i++){
			processesToSort2[i] = &nullProcess;
		}

		//check if all processes are finished, break if they are all finished
		if(numberOfFinishedProcesses == numberOfProcesses){
				break;
		}

		schedulerTime++; //incerement scheduler time

		/*printf("%d (Queue: %d)     ", processors[0]->pid, processors[0]->currentQueue);
		printf("%d (Queue: %d)     ", processors[1]->pid, processors[1]->currentQueue);
		printf("%d (Queue: %d)     ", processors[2]->pid, processors[2]->currentQueue);
		printf("%d (Queue: %d)     ", processors[3]->pid, processors[3]->currentQueue);
		printf("Finished processes: %d/%d\n", numberOfFinishedProcesses, numberOfProcesses);*/


	} //end of while loop

	//calculate and print average wait time of all processes
	float averageWaitTime = 0;
	for(i = 0; i < numberOfProcesses; i++){
		averageWaitTime = averageWaitTime + processes[i].waitingTime;
	}
	averageWaitTime = averageWaitTime / numberOfProcesses;
	printf("Average waiting time                 : %.2f time units\n", averageWaitTime);

	//calculate and print average turnaround time of all processes
	float averageTurnaroundTime = 0;
	for(i = 0; i < numberOfProcesses; i++){
		averageTurnaroundTime = averageTurnaroundTime + (processes[i].endTime - processes[i].arrivalTime);
	}
	averageTurnaroundTime = averageTurnaroundTime / numberOfProcesses;
	printf("Average turnaround time              : %.2f units\n", averageTurnaroundTime);

	//print the time for all processes to finish
	printf("Time all processes finished          : %d\n", schedulerTime);
	
	//calculate and print average CPU utilization by the time all processes finished, uses counter of how many processors were in use at each step
	float averageProcessorUtilization;
	averageProcessorUtilization = (processorUseCounter * 100) / schedulerTime;
	printf("Average CPU utilization              : %.1f%%\n", averageProcessorUtilization);

	//print number of context switched of all processes (always 0 for FCFS scheduler since nothing is preempted)
	printf("Number of context switches           : %d\n", contextSwitchCounter);

	//find and print pid(s) of process(es) which finished last
	printf("PID(s) of last process(es) to finish :");

	for(i = 0; i < numberOfProcesses; i++){
		if(processes[i].endTime == schedulerTime){
			printf(" %d", processes[i].pid);
		}
	}

	//print new line to look nice
	printf("\n");

	return 0;

}