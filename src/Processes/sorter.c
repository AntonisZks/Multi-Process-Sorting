/* Filename: sorter.c */

#include "../../include/sorter.h"

void SRT_init(SRT_process* process, SRT_data* process_data)
{
    process->read_fd  = process_data->read_fd;
    process->write_fd = process_data->write_fd;

    process->processId = getpid();

    DataFrom_WSRM receivedData;
    read(process->read_fd, &receivedData, sizeof(DataFrom_WSRM));

    process->numberOfRecords = receivedData.numberOfRecords;
    process->inputFileName = receivedData.inputFileName;

    process->sortingAlgorithm = receivedData.sortingAlgorithm;

    process->recordStartIndex = receivedData.recordStartIndex;
    process->recordEndIndex = receivedData.recordEndIndex;
}

void SRT_run(SRT_process* process)
{
    printf("Hello, from the sorter %d\n", getpid());
}

void SRT_print(SRT_process* process)
{
    printf("\nHello from the Sorter %d with:\n", process->processId);

    printf("- Input File Name: %s\n", process->inputFileName);
    printf("- Number of records in the file: %d\n", process->numberOfRecords);

    printf("- Record Starting Index: %d\n", process->recordStartIndex);
    printf("- Record End Index: %d\n", process->recordEndIndex);

    printf("- Sorting Algorithm: %s\n", process->sortingAlgorithm);
}
