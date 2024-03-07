/* Filename: workSpliterResultMerger.c */

#include "../../include/workSpliterResultMerger.h"

void WSRM_init(WSRM_process* process, WSRM_data* process_data)
{
    if ((process->childProcessesIds = (pid_t*)malloc(sizeof(pid_t) * process->numberofChildProcesses)) == NULL) { perror("Memory Error"); exit(1); }

    if ((process->parent_to_child_fd = (int**)malloc(sizeof(int*) * process->numberofChildProcesses)) == NULL) { perror("Memory Error"); exit(1); }
    if ((process->child_to_parent_fd = (int**)malloc(sizeof(int*) * process->numberofChildProcesses)) == NULL) { perror("Memory Error"); exit(1); }
    
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if ((process->parent_to_child_fd[i] = (int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
        if ((process->child_to_parent_fd[i] = (int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
    }

    process->read_fd = process_data->read_fd;
    process->write_fd = process_data->write_fd;

    
    DataFrom_CSMR receivedData;
    read(process->read_fd, &receivedData, sizeof(DataFrom_CSMR));

    process->inputFileName = receivedData.inputFileName;
    process->numberofChildProcesses = receivedData.numberofChildProcesses;

    process->sortingAlgorithm1 = receivedData.sortingAlgorithm1;
    process->sortingAlgorithm2 = receivedData.sortingAlgorithm2;

    process->processId = getpid();
    process->numberOfRecords = receivedData.numberOfRecords;

    process->recordStartIndex = receivedData.recordStartIndex;
    process->recordEndIndex = receivedData.recordEndIndex;
}

void WSRM_delete(WSRM_process* process)
{
    free(process->childProcessesIds);

    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        free(process->parent_to_child_fd[i]);
        free(process->child_to_parent_fd[i]);
    }

    free(process->parent_to_child_fd);
    free(process->child_to_parent_fd);
}

void WSRM_run(WSRM_process* process)
{
    pid_t wpid;
    int status;

    // Create the pipes for intel process communication
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if (pipe(process->parent_to_child_fd[i]) == -1 || pipe(process->child_to_parent_fd[i]) == -1) { // Checking for errors
            perror("Error creating communication pipes");
            exit(1);
        }
    }

    // Create the child processes for the Work-Spliter and Result-Merger process
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++)
    {
        if ((process->childProcessesIds[i] = fork()) == -1) {
            perror("Error creating chidl process");
            exit(1);
        }
        
        /* CODE FOR THE CHILD PROCESS */
        else if (process->childProcessesIds[i] == 0)
        {
            // Close any unused pipes
            for (unsigned int j = 0; j < process->numberofChildProcesses; j++) {
                close(process->parent_to_child_fd[j][WRITE_END]);
                close(process->child_to_parent_fd[j][READ_END]);
            }

            SRT_process childProcess;
            SRT_data childProcessData = 
            {
                process->numberOfRecords,
                process->inputFileName,
                process->sortingAlgorithm1,
                process->parent_to_child_fd[i][READ_END],
                process->child_to_parent_fd[i][WRITE_END]
            };

            SRT_init(&childProcess, &childProcessData);
            //SRT_print(&childProcess);
            SRT_run(&childProcess);


            // Close the pipe ends used for communication with the parent
            close(process->parent_to_child_fd[i][READ_END]);
            close(process->child_to_parent_fd[i][WRITE_END]);

            return;
        }
    }

    /* CODE FOR THE PARENT PROCESS */

    // Close any unsed pipes
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        close(process->parent_to_child_fd[i][READ_END]);
        close(process->child_to_parent_fd[i][WRITE_END]);
    }

    // Initialize the array to store the records' range, each child process is going to sort
    unsigned int** recordsRangeToSort = (unsigned int**)malloc(sizeof(unsigned int*) * process->numberofChildProcesses);
    if (recordsRangeToSort == NULL) {
        perror("Memory Error"); 
        exit(1);
    }
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if ((recordsRangeToSort[i] = (unsigned int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
    }

    // Construct the array
    unsigned int totalRecords = process->recordEndIndex - process->recordStartIndex + 1;
    unsigned int childProcess = process->numberofChildProcesses;
    unsigned int recordsOfEachProcess = totalRecords / childProcess;
    unsigned int extraRecords = totalRecords - (recordsOfEachProcess * childProcess);

    unsigned int counter = process->recordStartIndex, extraRecordsCounter = 0;
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++)
    {
        recordsRangeToSort[i][0] = counter;
        recordsRangeToSort[i][1] = counter + recordsOfEachProcess - 1;

        // Adding  an extra record for sorting, to the first processes
        if (extraRecordsCounter < extraRecords) {
            recordsRangeToSort[i][1]++;
            counter++;
            extraRecordsCounter++;
        }

        counter += recordsOfEachProcess;
    }

    // Sending the appropriate data to the child processes
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        DataTo_Sorter dataToSend = { recordsRangeToSort[i][0], recordsRangeToSort[i][1] };
        write(process->parent_to_child_fd[i][WRITE_END], &dataToSend, sizeof(DataTo_Sorter));
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

    // Deallocate the memory for the records's range array
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        free(recordsRangeToSort[i]);
    }
    free(recordsRangeToSort);

    printf("Hello, from the work-spliter %d with number of records: %d\n", process->processId, process->recordEndIndex - process->recordStartIndex + 1);
}

void WSRM_print(WSRM_process* process)
{
    printf("\nHello from the Work-Spliter and Result-Merger %d with:\n", process->processId);

    printf("- Input File Name: %s\n", process->inputFileName);
    printf("- Number of records in the file: %d\n", process->numberOfRecords);

    printf("- Number of child processes: %d\n", process->numberofChildProcesses);
    printf("- Record Starting Index: %d\n", process->recordStartIndex);
    printf("- Record End Index: %d\n", process->recordEndIndex);

    printf("- Sorting Algorithm 1: %s\n", process->sortingAlgorithm1);
    printf("- Sorting Algorithm 2: %s\n\n", process->sortingAlgorithm2);
}
