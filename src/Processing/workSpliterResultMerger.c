/* Filename: workSpliterResultMerger.c */

#include "../../include/workSpliterResultMerger.h"

void WSRM_init(WSRM_process* process, WSRM_data* process_data)
{
    process->inputFileName = process_data->inputFileName;
    process->numberofChildProcesses = process_data->numberofChildProcesses;

    process->sortingAlgorithm1 = process_data->sortingAlgorithm1;
    process->sortingAlgorithm2 = process_data->sortingAlgorithm2;

    process->processId = getpid();
    process->numberOfRecords = process_data->numberOfRecords;
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
            // Close any unsed pipe ends
            close(process->parent_to_child_fd[i][WRITE_END]);
            close(process->child_to_parent_fd[i][READ_END]);

            SRT_process childProcess;
            SRT_data childProcessData = { process->parent_to_child_fd[i][READ_END], process->child_to_parent_fd[i][WRITE_END] };

            SRT_init(&childProcess, &childProcessData);
            SRT_run(&childProcess);

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

    // Initialize the array to store the records' range, each child process is going to sort
    unsigned int** recordsRangeToSort = (unsigned int**)malloc(sizeof(int*) * process->numberofChildProcesses);
    if (recordsRangeToSort == NULL) {
        perror("Memory Error");
        exit(1);
    }
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if ((recordsRangeToSort[i] = (unsigned int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
    }

    // Initialize the array to store the sorting method of each sorter
    char** sortingMethods = (char**)malloc(sizeof(char*) * process->numberofChildProcesses);
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if (i % 2 == 0) sortingMethods[i] = process->sortingAlgorithm1;
        else            sortingMethods[i] = process->sortingAlgorithm2;
    }

    // Construct the array
    unsigned int totalRecords = process->recordEndIndex - process->recordStartIndex + 1;
    unsigned int childProcesses = process->numberofChildProcesses;
    unsigned int recordsOfEachProcess = totalRecords / childProcesses;
    unsigned int extraRecords = totalRecords - (recordsOfEachProcess * childProcesses);

    unsigned int counter = process->recordStartIndex, extraRecordsCounter = 0;
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++)
    {
        recordsRangeToSort[i][0] = counter;
        recordsRangeToSort[i][1] = counter + recordsOfEachProcess - 1;

        // Adding an extra record for sorting, to the first process
        if (extraRecordsCounter < extraRecords) {
            counter++;
            extraRecordsCounter++;
            recordsRangeToSort[i][1]++;
        }

        counter += recordsOfEachProcess;
    }

    // Sending the appropriate data to the child processes
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        DataTo_Srt dataToSend = 
        {
            process->numberOfRecords,
            process->inputFileName,
            sortingMethods[i],
            recordsRangeToSort[i][0],
            recordsRangeToSort[i][1]
        };

        if (write(process->parent_to_child_fd[i][WRITE_END], &dataToSend, sizeof(DataTo_Srt)) == -1) {
            perror("Error Writing Data");
            exit(1);
        }
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

    // Deallocate the memory used for the sorting methods array
    free(sortingMethods);

    // Deallocate the memory used for the records' range arrays
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        free(recordsRangeToSort[i]);
    }
    free(recordsRangeToSort);
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
