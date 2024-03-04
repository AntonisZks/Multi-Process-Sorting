/* Filename: coordinatorSpliterMergerReporter.h */

#ifndef COORDINATOR_SPLITER_MERGER_REPORTER_H
#define COORDINATOR_SPLITER_MERGER_REPORTER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct CoordinatorSpliterMergerReporterData
{
    unsigned int numberofChildProcesses;
    char* inputFileName;
    char* sortingAlgorithm1;
    char* sortingAlgorithm2;

} CSMR_data;

/**
 * @brief Structure that represents the Coordinator-Spliter and Result-Merger process
 * @author Antonis Zikas
 * @since 04/03/2024
 */
typedef struct CoordinatorSpliterMergerReporter
{
    unsigned int numberofChildProcesses;
    unsigned int numberOfRecords;
    
    pid_t* childProcessesIds;
    pid_t  processId;

    char* inputFileName;
    char* sortingAlgorithm1;
    char* sortingAlgorithm2;

} CSMR_process;

void CSMR_init(CSMR_process* process, CSMR_data* process_data);
void CSMR_run(CSMR_process* process);

void CSMR_print(CSMR_process* process);

#endif /* COORDINATOR_SPLITER_MERGER_REPORTER_H */
