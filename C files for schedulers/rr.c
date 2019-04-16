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
#include "sch-helpers.h"

process processes[MAX_PROCESSES + 1];          // a large structure array to hold all processes read from data file 
int numberOfProcesses = 0;                     // total number of processes 
process_queue readyQueue;  					   // a FIFO structure for the ready queue
process *processors[NUMBER_OF_PROCESSORS];     // an array of pointers for each of the four processors
process *waitingProcesses[MAX_PROCESSES];      // an array of pointers for processes that are doing an IO burst
int numberOfFinishedProcesses = 0;			   // counter for the number of processes which have finished
process nullProcess; 						   //a process to show an empty spot in a CPU
process_node *temp;							   // a pointer to a temporarily held process, used for traversing ready queue 
float processorUseCounter = 0;				   // counter for how many processors are in use (total)
int timeQuantumSpecified;
int contextSwitchCounter = 0;

//comparing function to order processes by lowest to highest pid if they have the same scheduling criterion
int compareByPid(const void *aa, const void *bb) {
    process *a = *(process**) aa;
    process *b = *(process**) bb;
    if (a->pid < b->pid) return -1;
    if (a->pid > b->pid) return 1;
    return 0;
}

int main (int argc, char *argv[]) {

	if(argc != 2){
		printf("USAGE: rr TIME-QUANTUM < CPU-LOAD-FILE\n");
		printf("  eg. \"rr 8 < load\" for time quantum 8 units processing workload in file `load'\n");
		return 0;
	}

	timeQuantumSpecified = atoi(argv[1]);

	if(timeQuantumSpecified < 1){
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
	process *processesToSort[numberOfProcesses];
	for(i = 0; i < numberOfProcesses; i++){
		processesToSort[i] = &nullProcess;
	}

	initializeProcessQueue(&readyQueue); //initialize ready queue
	int schedulerTime = 0; //start the timer of the scheduler at 0

	//start of while loop that will loop until all processes are finished
	while(1){

		//go through processes and check if any new processes have arrived, then add to processesToSort so they will be sorted and then added to the ready queue from lowest to highest pid
		for(i = 0; i < numberOfProcesses; i++){
			if(processes[i].arrivalTime == schedulerTime){

				for(secondCounter = 0; secondCounter < numberOfProcesses; secondCounter++){
					if(processesToSort[secondCounter]->pid == -1){
						processesToSort[secondCounter] = &processes[i];
						break;
					} //end of if
				} //end of inner for loop

			} //end of if
		} //end of outter for loop

		//check if any preocessors are free and add process to them if they are free (if any processes available)
		//also set the time quantum for processes being put into a CPU
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){

			if(processors[i]->pid == -1){
				if(readyQueue.size > 0){
					processors[i] = readyQueue.front->data;
					processors[i]->quantumRemaining = timeQuantumSpecified; //added in rr.c
					dequeueProcess(&readyQueue);
					if(processors[i]->currentBurst == 0){
						processors[i]->startTime = schedulerTime;
					} //end of if
				} //end of if
			} //end of if

		} //end of for loop

		//increment all processes step count which are in CPUs, as well as decrement the remaining quantum time for all processes in CPUs
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

		//increment wait time of all processes in ready queue
		if(readyQueue.size > 0){
			temp = readyQueue.front;
			while(temp != NULL){
				temp->data->waitingTime++;
				temp = temp->next;
			} //end of while loop
		} //end of if

		//check if any waiting processes done io burst, if they are send to processesToSort queue to be sorted before being put into the ready queue
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
							if(processesToSort[secondCounter]->pid == -1){
								processesToSort[secondCounter] = waitingProcesses[i];
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
								//waitingProcesses[j]->currentBurst++;
								processors[i] = &nullProcess;
								break;
							} //end of if
						} //end of inner for loop
					} //end of else

				} //end of if
			} //end of if

		} //end of outter for loop

		//check if any processes have been in a processor for the time quantum specified, if they have send to processesToSort to be put into ready queue
		//all added in rr.c
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
			if(processors[i]->pid != -1){

				if(processors[i]->quantumRemaining == 0){
					for(secondCounter = 0; secondCounter < numberOfProcesses; secondCounter++){
						if(processesToSort[secondCounter]->pid == -1){

							processesToSort[secondCounter] = processors[i];
							processors[i] = &nullProcess;
							contextSwitchCounter++;
							break;

						}//end of if
					}//end of inner for loop
				} //end of if

			} //end of if
		} //end of outter for loop

		//sort array processesToSort from lowest pid to highest pid
		qsort(processesToSort, numberOfProcesses, sizeof(process *), &compareByPid);

		//enqeue all processes in processesToSort now that they have been sorted
		for(i = 0; i < numberOfProcesses; i++){
			if(processesToSort[i]->pid != -1){
				enqueueProcess(&readyQueue, processesToSort[i]);
			}
		}
		//make all spots in processesToSort point to the nullProcess since all processes have been put into the ready queue
		for(i = 0; i < numberOfProcesses; i++){
			processesToSort[i] = &nullProcess;
		}

		//check if all processes are finished, break if they are all finished
		if(numberOfFinishedProcesses == numberOfProcesses){
				break;
		}

		schedulerTime++; //incerement scheduler time

	}

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