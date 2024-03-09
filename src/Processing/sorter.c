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
    char programToRun[100];
    char* argv[6];
    char filename[50], recordsCount[20], startIndex[20], endIndex[20];

    // Checking if the start and end index are valid (start should be lower than or equal to end)
    if (process->recordStartIndex > process->recordEndIndex) {
        return;
    }

    sprintf(programToRun, "./bin/%s", process->sortingAlgorithm);

    strcpy(filename, process->inputFileName);
    sprintf(recordsCount, "%d", process->numberOfRecords);
    sprintf(startIndex, "%d", process->recordStartIndex);
    sprintf(endIndex, "%d", process->recordEndIndex);

    argv[0] = programToRun;
    argv[1] = filename;
    argv[2] = recordsCount;
    argv[3] = startIndex;
    argv[4] = endIndex;
    argv[5] = NULL;

    dup2(process->write_fd, STDOUT_FILENO);
    execv(programToRun, argv);

    perror("Sorting Execution Error");
    exit(1);
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
