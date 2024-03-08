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
    // Initialize the input filename and the child processes count
    process->inputFileName = process_data->inputFileName;
    process->numberofChildProcesses = process_data->numberofChildProcesses;
    
    // Initialize the sorting algorithms
    process->sortingAlgorithm1 = process_data->sortingAlgorithm1;
    process->sortingAlgorithm2 = process_data->sortingAlgorithm2;

    // Initialize the process id and the records count in the input file
    process->processId = getpid();
    process->numberOfRecords = getRecordsCountInFile(process->inputFileName);
    if ((process->childProcessesIds = (pid_t*)malloc(sizeof(pid_t) * process->numberofChildProcesses)) == NULL) { perror("Memory Error"); exit(1); }

    // Initialize the pipes for intel-process communication
    if ((process->parent_to_child_fd = (int**)malloc(sizeof(int*) * process->numberofChildProcesses)) == NULL) { perror("Memory Error"); exit(1); }
    if ((process->child_to_parent_fd = (int**)malloc(sizeof(int*) * process->numberofChildProcesses)) == NULL) { perror("Memory Error"); exit(1); }

    // Allocating memory for the pipes communication arrays
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if ((process->parent_to_child_fd[i] = (int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
        if ((process->child_to_parent_fd[i] = (int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
    }
}

void CSMR_delete(CSMR_process* process)
{
    free(process->childProcessesIds);

    // Deallocating the memory used for the pipwa communication system
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        free(process->parent_to_child_fd[i]);
        free(process->child_to_parent_fd[i]);
    }

    free(process->parent_to_child_fd);
    free(process->child_to_parent_fd);
}

void CSMR_run(CSMR_process* process)
{
    pid_t wpid;
    int status;

    // Create the pipes for intel process communication
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if (pipe(process->parent_to_child_fd[i]) == -1 || pipe(process->child_to_parent_fd[i]) == -1) { // Check for any errors
            perror("Error creating communication pipes");
            exit(1);
        }
    }

    // Create the child processes of the Coordinator-Spliter and Result-Merger process
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++)
    {
        if ((process->childProcessesIds[i] = fork()) == -1) { // Check for errors while creating
            perror("Error creating child process");
            exit(1);
        }

        /* CODE FOR THE CHILD PROCESS */
        else if (process->childProcessesIds[i] == 0) 
        {
            // Close any unused pipe ends
            for (unsigned int j = 0; j < process->numberofChildProcesses; j++) {
                close(process->parent_to_child_fd[j][WRITE_END]);
                close(process->child_to_parent_fd[j][READ_END]);
            }

            WSRM_process childProcess;
            WSRM_data childProcessData = 
            { 
                process->numberofChildProcesses - i, 
                process->numberOfRecords,
                process->inputFileName,
                process->sortingAlgorithm1,
                process->sortingAlgorithm2,
                process->parent_to_child_fd[i][READ_END],
                process->child_to_parent_fd[i][WRITE_END]
            };

            WSRM_init(&childProcess, &childProcessData);
            WSRM_run(&childProcess);

            WSRM_delete(&childProcess);

            // Close the pipe ends used for communication with the parent
            close(process->parent_to_child_fd[i][READ_END]);
            close(process->child_to_parent_fd[i][WRITE_END]);

            return;
        }
    }

    /* CODE FOR THE PARENT PROCESS */

    // Close any unused pipe ends
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        close(process->parent_to_child_fd[i][READ_END]);
        close(process->child_to_parent_fd[i][WRITE_END]);
    }

    // Initialize an array to store the records' range, each child process is going to sort
    unsigned int** recordsRangeToSort = (unsigned int**)malloc(sizeof(unsigned int*) * process->numberofChildProcesses);
    if (recordsRangeToSort == NULL) {
        perror("Memory Error"); 
        exit(1);
    }
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if ((recordsRangeToSort[i] = (unsigned int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
    }

    // Construct the array
    unsigned int totalRecords = process->numberOfRecords;
    unsigned int childProcesses = process->numberofChildProcesses;
    unsigned int recordsOfEachProcess = totalRecords / childProcesses;
    unsigned int extraRecords = totalRecords - (recordsOfEachProcess * childProcesses);

    unsigned int counter = 0, extraRecordsCounter = 0;
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        recordsRangeToSort[i][0] = counter;
        recordsRangeToSort[i][1] = counter + recordsOfEachProcess - 1;
        
        // Adding an extra record for sorting, to the first processes
        if (extraRecordsCounter < extraRecords) {
            recordsRangeToSort[i][1]++;
            counter++;
            extraRecordsCounter++;
        }

        counter += recordsOfEachProcess;
    }

    // Sending the appropriate data to the child processes
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        DataTo_WSRM dataToSend = { recordsRangeToSort[i][0], recordsRangeToSort[i][1] };
        write(process->parent_to_child_fd[i][WRITE_END], &dataToSend, sizeof(DataTo_WSRM));
    }

    // Wait for all the child processes to finish executing
    while ((wpid = waitpid(-1, &status, 0)) > 0) {
        if (!WIFEXITED(status)) {
            printf("Child Process %d terminated abnormally\n", wpid);
        }
    }

    // Close the pipe ends used for communication with the child processes
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        close(process->parent_to_child_fd[i][WRITE_END]);
        close(process->child_to_parent_fd[i][READ_END]);
    }

    // Deallocate the memory for the records' range array
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        free(recordsRangeToSort[i]);
    }
    free(recordsRangeToSort);
}

void CSMR_print(CSMR_process* process)
{
    printf("\nHello from the Coordinator-Spliter and Merger-Reporter %d with:\n", process->processId);

    printf("- Input File Name: %s\n", process->inputFileName);
    printf("- Number of records in the file: %d\n", process->numberOfRecords);

    printf("- Number of child processes: %d\n", process->numberofChildProcesses);

    printf("- Sorting Algorithm 1: %s\n", process->sortingAlgorithm1);
    printf("- Sorting Algorithm 2: %s\n\n", process->sortingAlgorithm2);
}
