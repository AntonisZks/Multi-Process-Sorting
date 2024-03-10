/* Filename: coordinatorSpliterMergerReporter.c */

#include "../../include/record.h"
#include "../../include/coordinatorSpliterMergerReporter.h"

static void merge(Record* leftArray, unsigned int leftArraySize, Record* rightArray, unsigned int rightArraySize, Record* resultArray)
{
    unsigned int leftArrayIndex = 0, rightArrayIndex = 0, resultArrayIndex = 0;

    while (leftArrayIndex < leftArraySize && rightArrayIndex < rightArraySize)
    {
        if (isLowerThan(leftArray[leftArrayIndex], rightArray[rightArrayIndex])) resultArray[resultArrayIndex++] = leftArray[leftArrayIndex++];
        else resultArray[resultArrayIndex++] = rightArray[rightArrayIndex++];
    }

    while (leftArrayIndex  < leftArraySize)  resultArray[resultArrayIndex++] = leftArray[leftArrayIndex++];
    while (rightArrayIndex < rightArraySize) resultArray[resultArrayIndex++] = rightArray[rightArrayIndex++];
}

static Record* mergeRecords(CSMR_process process, Record** records, unsigned int* recordsCounts)
{
    Record* mergedRecords = (Record*)malloc(sizeof(Record) * process.processRecords);

    // Check if there is only one child process. If so the sorted records from that, is the result merged array
    if (process.numberofChildProcesses == 1) {
        for (unsigned int i = 0; i < process.numberOfRecords; i++) {
            mergedRecords[i] = records[0][i];
        }

        return mergedRecords;
    }

    unsigned int leftArraySize  = recordsCounts[0];
    unsigned int rightArraySize = recordsCounts[1];
    
    Record* leftArray  = (Record*)malloc(sizeof(Record) * leftArraySize);
    Record* rightArray = (Record*)malloc(sizeof(Record) * rightArraySize);
    Record* result     = (Record*)malloc(sizeof(Record) * (leftArraySize + rightArraySize));


    for (unsigned int i = 0; i < recordsCounts[0]; i++) leftArray[i]  = records[0][i];
    for (unsigned int i = 0; i < recordsCounts[1]; i++) rightArray[i] = records[1][i];

    for (unsigned int i = 0; i < process.numberofChildProcesses - 1; i++)
    {
        merge(leftArray, leftArraySize, rightArray, rightArraySize, result);

        if (i < process.numberofChildProcesses - 2) 
        {
            leftArraySize += rightArraySize;
            leftArray = (Record*)realloc(leftArray, sizeof(Record) * leftArraySize);
            for (unsigned int j = 0; j < leftArraySize; j++) {
                leftArray[j] = result[j];
            }

            rightArraySize = recordsCounts[i + 2];
            rightArray = (Record*)realloc(rightArray, sizeof(Record) * rightArraySize);
            for (unsigned int j = 0; j < rightArraySize; j++) {
                rightArray[j] = records[i + 2][j];
            }
            
            unsigned int newResultArraySize = leftArraySize + rightArraySize;
            result = (Record*)realloc(result, sizeof(Record) * newResultArraySize);
        }
    }

    for (unsigned int i = 0; i < process.processRecords; i++) {
        mergedRecords[i] = result[i];
    }

    free(leftArray);
    free(rightArray);
    free(result);

    return mergedRecords;
}

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
    process->processRecords = process->numberOfRecords;
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
                i + 1,
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

    // Reading the sorted records the work-spliter and result-merger has sent back to the cooridinator-spliter
    Record** sortedRecords = (Record**)malloc(sizeof(Record*) * process->numberofChildProcesses);
    unsigned int* recordsCounts = (unsigned int*)malloc(sizeof(unsigned int) * process->numberofChildProcesses);
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        sortedRecords[i] = (Record*)malloc(sizeof(Record) * (recordsRangeToSort[i][1] - recordsRangeToSort[i][0] + 1));
    }

    for (unsigned int i = 0; i < process->numberofChildProcesses; i++)
    {
        unsigned int recordsCount = (recordsRangeToSort[i][1] - recordsRangeToSort[i][0] + 1);
        recordsCounts[i] = recordsCount;

        // Reading every record from the current child process
        for (unsigned int j = 0; j < recordsCount; j++) {
            Record currentRecord;
            read(process->child_to_parent_fd[i][READ_END], &currentRecord, sizeof(Record));

            sortedRecords[i][j] = currentRecord;
        }
    }

    Record* mergedRecords = mergeRecords(*process, sortedRecords, recordsCounts);

    for (unsigned int i = 0; i < process->processRecords; i++) {
        printRecord(mergedRecords[i]);
    }

    free(mergedRecords);
    free(recordsCounts);

    // Close the pipe ends used for communication with the child processes
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        close(process->parent_to_child_fd[i][WRITE_END]);
        close(process->child_to_parent_fd[i][READ_END]);
    }

    // Deallocate the memory used for the sorted records arrays
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        free(sortedRecords[i]);
    }
    free(sortedRecords);


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
