/* Filename: sorter.h */

#ifndef SORTER_H
#define SORTER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @brief Structure that represents the data, the Work-Spliter and Result-Merger 
 * is going to receive from the Coordinator-Spliter and Merger-Reporter
 * 
 * @author Antonis Zikas
 * @since 05/03/2024
 */
typedef struct DataFromWorkSpliterResultMerger
{
    unsigned int recordStartIndex;
    unsigned int recordEndIndex;

} DataFrom_WSRM;

/**
 * @brief Structure that represents the data of the sorter
 * @author Antonis Zikas
 * @since 05/03/2024
 */
typedef struct SorterData
{
    unsigned int numberOfRecords;

    char* inputFileName;
    char* sortingAlgorithm;

    int read_fd, write_fd;

} SRT_data;

/**
 * @brief Structure that represents the Sorter process
 * @author Antonis Zikas
 * @since 05/03/2024
 */
typedef struct Sorter
{
    unsigned int numberOfRecords;

    pid_t processId;

    char* inputFileName;
    char* sortingAlgorithm;

    unsigned int recordStartIndex;
    unsigned int recordEndIndex;

    int read_fd, write_fd;

} SRT_process;

void SRT_init(SRT_process* process, SRT_data* process_data);
void SRT_run(SRT_process* process);

void SRT_print(SRT_process* process);

#endif /* SORTER_H */
