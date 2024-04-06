/* Filename: workSpliterResultMerger.c */

#include "../../include/workSpliterResultMerger.h"

/**
 * @brief Merges the records from two arrays into a final result array
 * 
 * @param leftArray the first array of the merge process
 * @param leftArraySize the size of the first array
 * @param rightArray the second array of the merge process
 * @param rightArraySize the size of the second array
 * @param resultArray the final merged Array
 */
static void merge(Record* leftArray, unsigned int leftArraySize, Record* rightArray, unsigned int rightArraySize, Record* resultArray)
{
    unsigned int leftArrayIndex = 0, rightArrayIndex = 0, resultArrayIndex = 0;

    while (leftArrayIndex < leftArraySize && rightArrayIndex < rightArraySize)
    {
        Record leftRecord  = leftArray[leftArrayIndex];
        Record rightRecord = rightArray[rightArrayIndex];

        if (isLowerThan(leftRecord, rightRecord)) {
            resultArray[resultArrayIndex++] = leftArray[leftArrayIndex++];
        } else {
            resultArray[resultArrayIndex++] = rightArray[rightArrayIndex++];
        }
    }

    while (leftArrayIndex < leftArraySize) {
        resultArray[resultArrayIndex++] = leftArray[leftArrayIndex++];
    }

    while (rightArrayIndex < rightArraySize) {
        resultArray[resultArrayIndex++] = rightArray[rightArrayIndex++];
    }
}

/**
 * @brief Merges all the sorted records from the different child processes into one result array
 * 
 * @param process the current process of the application
 * @param records the array containing all the sorted records of all the child processes
 * @param recordsCounts the number of records each child process has sorted
 */
static Record* mergeRecords(WSRM_process process, Record** records, unsigned int* recordsCounts)
{
    Record* mergedRecords = (Record*)malloc(sizeof(Record) * process.processRecords);

    // Check if there is only one child process. If so the sorted records from that, is the result merged array
    if (process.numberofChildProcesses == 1) {
        for (unsigned int i = 0; i < process.numberOfRecords; i++) {
            mergedRecords[i] = records[0][i];
        }

        return mergedRecords;
    }

    // Else merge all the individual arrays into one general result array
    unsigned int leftArraySize  = recordsCounts[0];
    unsigned int rightArraySize = recordsCounts[1];

    // Construct the first pair of arrays
    Record* leftArray  = (Record*)malloc(sizeof(Record) * leftArraySize);
    Record* rightArray = (Record*)malloc(sizeof(Record) * rightArraySize);
    Record* result     = (Record*)malloc(sizeof(Record) * (leftArraySize + rightArraySize));

    if (leftArray == NULL || rightArray == NULL || result == NULL) { // Check for any errors
        perror("Memory Error");
        exit(1);
    }

    for (unsigned int i = 0; i < recordsCounts[0]; i++) leftArray[i]  = records[0][i];
    for (unsigned int i = 0; i < recordsCounts[1]; i++) rightArray[i] = records[1][i];

    // Merging Process
    for (unsigned int i = 0; i < process.numberofChildProcesses - 1; i++)
    {
        merge(leftArray, leftArraySize, rightArray, rightArraySize, result);

        if (i == process.numberofChildProcesses - 2) {
            break;
        }
        
        // Else continue the merge process by merging the next pair of arrays
        // Update the left array
        leftArraySize += rightArraySize;
        leftArray = (Record*)realloc(leftArray, sizeof(Record) * leftArraySize);
        if (leftArray == NULL) { // Check for any errors
            perror("Memory Error");
            exit(1);
        }

        for (unsigned int j = 0; j < leftArraySize; j++) {
            leftArray[j] = result[j];
        }

        // Update the right array
        rightArraySize = recordsCounts[i + 2];
        rightArray = (Record*)realloc(rightArray, sizeof(Record) * rightArraySize);
        if (rightArray == NULL) { // Check for any errors
            perror("Memory Error");
            exit(1);
        }
        
        for (unsigned int j = 0; j < rightArraySize; j++) {
            rightArray[j] = records[i + 2][j];
        }
        
        // Update the result array
        unsigned int newResultArraySize = leftArraySize + rightArraySize;
        result = (Record*)realloc(result, sizeof(Record) * newResultArraySize);
        if (result == NULL) { // Check for any errors
            perror("Memory Error");
            exit(1);
        }
    }

    // Copy the elements of the result array to the result array and return it
    for (unsigned int i = 0; i < process.processRecords; i++) {
        mergedRecords[i] = result[i];
    }

    // Free up memory
    free(leftArray);
    free(rightArray);
    free(result);

    return mergedRecords;
}

/**
 * @brief Initializes the Work-Spliter and Result-Merger by applying to it the appropriate data
 * 
 * @param process the current Work-Spliter and Result-Merger process
 * @param process_data the data of the current Work-Spliter and Result-Merger process
 */
void WSRM_init(WSRM_process* process, WSRM_data* process_data)
{
    // Initialize the input filename and the child processes count
    process->processIndex = process_data->processIndex;
    process->inputFileName = process_data->inputFileName;
    process->numberofChildProcesses = process_data->numberofChildProcesses;

    // Initialize the sorting algorithms
    process->sortingAlgorithm1 = process_data->sortingAlgorithm1;
    process->sortingAlgorithm2 = process_data->sortingAlgorithm2;

    // Initialize the process id and the records count in the input file
    process->processId = getpid();
    process->numberOfRecords = process_data->numberOfRecords;

    // Initialize arrays for the child ids, and the pipes for intel-process communication
    process->childProcessesIds = (pid_t*)malloc(sizeof(pid_t) * process->numberofChildProcesses);
    process->parent_to_child_fd = (int**)malloc(sizeof(int*)  * process->numberofChildProcesses);
    process->child_to_parent_fd = (int**)malloc(sizeof(int*)  * process->numberofChildProcesses);

    if (!process->childProcessesIds || !process->parent_to_child_fd || !process->child_to_parent_fd) { // Check for any errors
        perror("Memory Error"); 
        exit(1); 
    }
    
    // Allocating memory for the pipes communication arrays
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        if ((process->parent_to_child_fd[i] = (int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
        if ((process->child_to_parent_fd[i] = (int*)malloc(sizeof(int) * 2)) == NULL) { perror("Memory Error"); exit(1); }
    }

    process->read_fd  = process_data->read_fd;
    process->write_fd = process_data->write_fd;

    DataFrom_CSMR receivedData;
    read(process->read_fd, &receivedData, sizeof(DataFrom_CSMR));

    process->recordStartIndex = receivedData.recordStartIndex;
    process->recordEndIndex   = receivedData.recordEndIndex;
    process->processRecords   = receivedData.recordEndIndex - receivedData.recordStartIndex + 1;
}

/**
 * @brief Deletes the memory used for the Work-Spliter and Result-Merger process
 * @param process the current Work-Spliter and Result-Merger process
 */
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

/**
 * @brief Runs the Work-Spliter and Result-Merger process
 * @param process the current Work-Spliter and Result-Merger process
 */
void WSRM_run(WSRM_process* process)
{
    // sleep(process->processIndex);
    // printf("\n-----Work-Splitter %d is running-----\n\n", process->processIndex);

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
            perror("Error creating child process");
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
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++){
        DataTo_Srt dataToSend = { 
            process->numberOfRecords, process->inputFileName, sortingMethods[i], recordsRangeToSort[i][0], recordsRangeToSort[i][1] 
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

    // Reading the sorted records the sorter has sent back to the work-spliter
    Record** sortedRecords = (Record**)malloc(sizeof(Record*) * process->numberofChildProcesses);
    unsigned int* recordsCounts = (unsigned int*)malloc(sizeof(unsigned int) * process->numberofChildProcesses);
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        sortedRecords[i] = (Record*)malloc(sizeof(Record) * (recordsRangeToSort[i][1] - recordsRangeToSort[i][0] + 1));
    }

    for (unsigned int i = 0; i < process->numberofChildProcesses; i++)
    {
        unsigned int recordsCount = (recordsRangeToSort[i][1] - recordsRangeToSort[i][0] + 1); // Number of records of the current child process (sorter)
        recordsCounts[i] = recordsCount;

        // Reading every records from the current child process
        for (unsigned int j = 0; j < recordsCount; j++) {
            Record currentRecord;
            read(process->child_to_parent_fd[i][READ_END], &currentRecord, sizeof(Record));

            sortedRecords[i][j] = currentRecord;
        }
    }

    Record* mergedRecords = mergeRecords(*process, sortedRecords, recordsCounts); 
    
    // Send to merged records back to the parent process
    dup2(process->write_fd, STDOUT_FILENO);
    for (unsigned int i = 0; i < process->processRecords; i++) {
        write(STDOUT_FILENO, &mergedRecords[i], sizeof(Record));
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

    // Deallocate the memory used for the sorting methods array
    free(sortingMethods);

    // Deallocate the memory used for the records' range arrays
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) {
        free(recordsRangeToSort[i]);
    }
    free(recordsRangeToSort);
}

/**
 * @brief Represents the Work-Spliter and Result-Merger process
 * @param process the current Work-Spliter and Result-Merger process
 */
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
