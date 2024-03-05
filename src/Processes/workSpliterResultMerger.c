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
    process->childProcessesIds = (pid_t*)malloc(sizeof(pid_t) * process->numberofChildProcesses);

    process->parent_to_child_fd = (int**)malloc(sizeof(int*) * process->numberofChildProcesses);
    process->child_to_parent_fd = (int**)malloc(sizeof(int*) * process->numberofChildProcesses);
    
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        process->parent_to_child_fd[i] = (int*)malloc(sizeof(int) * 2);
        process->child_to_parent_fd[i] = (int*)malloc(sizeof(int) * 2);
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
            SRT_run();
            return;
        }
    }

    /* CODE FOR THE PARENT PROCESS */

    // Wait for all the child processes to finish executing
    while ((wpid = waitpid(-1, &status, 0)) > 0) {
        if (!WIFEXITED(status)) {
            printf("Child Process %d terminated abnormally\n", wpid);
        }
    }

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
