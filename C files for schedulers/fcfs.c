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

//comparing function to order processes by lowest to highest pid if they have the same scheduling criterion
int compareByPid(const void *aa, const void *bb) {
    process *a = *(process**) aa;
    process *b = *(process**) bb;
    if (a->pid < b->pid) return -1;
    if (a->pid > b->pid) return 1;
    return 0;
}

int main (int argc, char *argv[]) {

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

	//printf("Number of processes imported: %d\n", numberOfProcesses);

	//print out processes for error checking and visual
	//printf("processes imported:\n");
	/*for(i = 0; i < (sizeof(processes) / sizeof(processes[0])); i++){
		printf("%d %d %d\n", processes[i].pid, processes[i].arrivalTime, processes[i].currentBurst);
	}
	printf("processors with pid:\n");
	for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
		printf("processor %d: pid is: %d and arrival time is: %d\n", i + 1, processors[i]->pid, processors[i]->arrivalTime);
	}*/

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
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){

			if(processors[i]->pid == -1){
				if(readyQueue.size > 0){
					processors[i] = readyQueue.front->data;
					dequeueProcess(&readyQueue);
					if(processors[i]->currentBurst == 0){
						processors[i]->startTime = schedulerTime;
					} //end of if
				} //end of if
			} //end of if

		} //end of for loop

		//increment all processes step count which are in CPUs
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){

			if(processors[i]->pid != -1){
				processors[i]->bursts[processors[i]->currentBurst].step++;
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

		/*printf("Time %d\n", schedulerTime);
		for(i = 0; i < numberOfProcesses; i++){
			printf("%d\n", processesToSort[i]->pid);
		}
		printf("\n");*/

		//sort array processesToSort from lowest pid to highest pid
		qsort(processesToSort, numberOfProcesses, sizeof(process *), &compareByPid);

		/*printf("Sorted: \n");
		for(i = 0; i < numberOfProcesses; i++){
			printf("%d\n", processesToSort[i]->pid);
		}
		printf("\n");*/

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
		
		/*if(readyQueue.size > 0){
			printf("%d processes in the ready queue and the head has waiting time %d\n", readyQueue.size, readyQueue.front->data->waitingTime);
		}
		else{
			printf("no processes waiting in ready queue\n");
		}
		printf("Number of finished processes: %d\n", numberOfFinishedProcesses);
		for(i = 0; i < numberOfProcesses; i++){
			printf("Waiting process %d has pid %d and has waited %d time units. time to get out is: %d\n", i, waitingProcesses[i]->pid, waitingProcesses[i]->bursts[waitingProcesses[i]->currentBurst].step, waitingProcesses[i]->bursts[waitingProcesses[i]->currentBurst].length);
			printf("Waiting process %d has pid %d. Current burst is: %d and number of bursts is: %d\n", i, waitingProcesses[i]->pid, waitingProcesses[i]->currentBurst, waitingProcesses[i]->numberOfBursts);

		}
		printf("processor %d: pid is: %d and arrival time is: %d and current step is: %d\n", 0, processors[0]->pid, processors[0]->arrivalTime, processors[0]->bursts[processors[0]->currentBurst].step);
		printf("processor %d: pid is: %d. Current burst is: %d and number of bursts is: %d\n", 0, processors[0]->pid, processors[0]->currentBurst, processors[0]->numberOfBursts);
		printf("processor %d: pid is: %d and arrival time is: %d and current step is: %d\n", 1, processors[1]->pid, processors[1]->arrivalTime, processors[1]->bursts[processors[1]->currentBurst].step);
		printf("processor %d: pid is: %d. Current burst is: %d and number of bursts is: %d\n", 1, processors[1]->pid, processors[1]->currentBurst, processors[1]->numberOfBursts);
		printf("processor %d: pid is: %d and arrival time is: %d and current step is: %d\n", 2, processors[2]->pid, processors[2]->arrivalTime, processors[2]->bursts[processors[2]->currentBurst].step);
		printf("processor %d: pid is: %d. Current burst is: %d and number of bursts is: %d\n", 2, processors[2]->pid, processors[2]->currentBurst, processors[2]->numberOfBursts);
		printf("processor %d: pid is: %d and arrival time is: %d and current step is: %d\n", 3, processors[3]->pid, processors[3]->arrivalTime, processors[3]->bursts[processors[3]->currentBurst].step);
		printf("processor %d: pid is: %d. Current burst is: %d and number of bursts is: %d\n", 3, processors[3]->pid, processors[3]->currentBurst, processors[3]->numberOfBursts);
		*/
		//endTime

		//check if all processes are finished, break if they are all finished
		if(numberOfFinishedProcesses == numberOfProcesses){
				break;
		}

		schedulerTime++; //incerement scheduler time

	} //end of while loop

	/*printf("%d\n", processes[0].waitingTime);
	printf("%d\n", processes[1].waitingTime);
	printf("%d\n", processes[2].waitingTime);
	printf("%d\n", processes[3].waitingTime);
	printf("%d\n", processes[4].waitingTime);
	printf("%d\n", processes[5].waitingTime);*/
	//for question 1

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
	printf("Number of context switches           : 0\n");

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