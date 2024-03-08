/* Filename: workSpliterResultMerger.h */

#ifndef WORK_SPLITER_RESULT_MERGER_H
#define WORK_SPLITER_RESULT_MERGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "sorter.h"

#define READ_END  (0)
#define WRITE_END (1)

/**
 * @brief Structure that represents the data, the Work-Spliter and Result-Merger 
 * is going to receive from the Coordinator-Spliter and Merger-Reporter
 * 
 * @author Antonis Zikas
 * @since 05/03/2024
 */
typedef struct DataFromCoordinatorSpliterMergerReporter
{
    unsigned int recordStartIndex;
    unsigned int recordEndIndex;

} DataFrom_CSMR;

/**
 * @brief Structure that represents the data of the Work-Spliter and Result-Merger
 * @author Antonis Zikas
 * @since 05/03/2024
 */
typedef struct WorkSpliterResultMergerData
{
    unsigned int numberofChildProcesses;
    unsigned int numberOfRecords;

    char* inputFileName;
    char* sortingAlgorithm1;
    char* sortingAlgorithm2;

    int read_fd, write_fd;

} WSRM_data;

/**
 * @brief Structure that represents the Work-Spliter and Result-Merger process type
 * @author Antonis Zikas
 * @since 05/03/2024 
 */
typedef struct WorkSpliterResultMerger
{
    unsigned int numberofChildProcesses;
    unsigned int numberOfRecords;

    pid_t* childProcessesIds;
    pid_t  processId;

    char* inputFileName;
    char* sortingAlgorithm1;
    char* sortingAlgorithm2;

    int** parent_to_child_fd;
    int** child_to_parent_fd;

    unsigned int recordStartIndex;
    unsigned int recordEndIndex;

    int read_fd, write_fd;

} WSRM_process;

void WSRM_init(WSRM_process* process, WSRM_data* process_data);
void WSRM_delete(WSRM_process* process);
void WSRM_run(WSRM_process* process);

void WSRM_print(WSRM_process* process);

#endif /* WORK_SPLITER_RESULT_MERGER_H */
