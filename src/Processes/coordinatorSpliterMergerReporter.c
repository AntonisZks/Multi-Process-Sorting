/* Filename: coordinatorSpliterMergerReporter.c */

#include "../../include/record.h"
#include "../../include/coordinatorSpliterMergerReporter.h"

static unsigned int getRecordsCountInFile(const char* filename)
{
    unsigned int numofRecords;
    long fileSize;

    // Open the given file for reading, and check for any errors
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening the given file for reading");
        exit(1);
    }

    // Fetch the size of the file in bytes
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);

    fclose(fp); // Make sure to close the file

    // Calculate and return the number of records in the file
    numofRecords = (int)fileSize/sizeof(Record);
    return numofRecords;
}

void CSMR_init(CSMR_process* process, CSMR_data* process_data)
{
    process->numberofChildProcesses = process_data->numberofChildProcesses;
    process->inputFileName = process_data->inputFileName;
    
    process->sortingAlgorithm1 = process_data->sortingAlgorithm1;
    process->sortingAlgorithm2 = process_data->sortingAlgorithm2;

    process->numberOfRecords = getRecordsCountInFile(process->inputFileName);
    process->processId = getpid();
}

void CSMR_run(CSMR_process* process)
{
    printf("Running...\n");
}

void CSMR_print(CSMR_process* process)
{
    printf("Hello from the Coordinator-Spliter and Merger-Reporter %d with:\n", process->processId);

    printf("Input File Name: %s\n", process->inputFileName);
    printf("Number of records in the file: %d\n", process->numberOfRecords);

    printf("Number of child processes: %d\n", process->numberofChildProcesses);

    printf("Sorting Algorithm 1: %s\n", process->sortingAlgorithm1);
    printf("Sorting Algorithm 2: %s\n", process->sortingAlgorithm2);
}
