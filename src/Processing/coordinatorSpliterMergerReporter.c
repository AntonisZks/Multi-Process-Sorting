/* Filename: coordinatorSpliterMergerReporter.c */

#include "../../include/record.h"
#include "../../include/coordinatorSpliterMergerReporter.h"

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

    while (leftArrayIndex  < leftArraySize) {
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
static Record* mergeRecords(CSMR_process process, Record** records, unsigned int* recordsCounts)
{
    Record* mergedRecords = (Record*)malloc(sizeof(Record) * process.processRecords);
    if (mergedRecords == NULL) {
        perror("Memory Error");
        exit(1);
    }

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

        // If all merges are done stop the iteration
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
 * @brief Executes the current child process code
 * 
 * @param process the current Coordinator-Spliter and Merger-Reporter process
 * @param childIndex the index of the current child process
 */
static void runChildProcess(CSMR_process* process, const unsigned int childIndex)
{
    // Close any unused pipe ends
    for (unsigned int j = 0; j < process->numberofChildProcesses; j++) {
        close(process->parent_to_child_fd[j][WRITE_END]);
        close(process->child_to_parent_fd[j][READ_END]);
    }

    // Construct the Work-Spliter and Result-Merger process and its data
    WSRM_process childProcess;
    WSRM_data childProcessData = {
        childIndex + 1,
        process->numberofChildProcesses - childIndex, 
        process->numberOfRecords,
        process->inputFileName,
        process->sortingAlgorithm1,
        process->sortingAlgorithm2,
        process->parent_to_child_fd[childIndex][READ_END],
        process->child_to_parent_fd[childIndex][WRITE_END]
    };

    // Initialize the process and run it
    WSRM_init(&childProcess, &childProcessData);
    WSRM_run(&childProcess);

    // Delete the memory used for the process
    WSRM_delete(&childProcess);

    // Close the pipe ends used for communication with the parent
    close(process->parent_to_child_fd[childIndex][READ_END]);
    close(process->child_to_parent_fd[childIndex][WRITE_END]);
}

/**
 * @brief Returns an array containg the records range for every child process to sort
 * 
 * @param process the current Coordinator-Spliter and Merger-Reporter process
 * @return a 2D array containing the range of records for every child process sorting
 */
static unsigned int** getRecordsRangeArray(CSMR_process* process)
{
    unsigned int** recordsRangeToSort = (unsigned int**)malloc(sizeof(unsigned int*) * process->numberofChildProcesses);
    if (recordsRangeToSort == NULL) { // Check for any errors
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
            recordsRangeToSort[i][1]++; extraRecordsCounter++; counter++;
        }

        counter += recordsOfEachProcess;
    }

    return recordsRangeToSort;
}

/**
 * @brief Returns the number of records inside the given file
 * 
 * @param filename the name of the file
 * @return the number of records inside that file
 */
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

/**
 * @brief Initializes the Coordinator-Splitter and Merger-Reporter by applying to it the appropriate data
 * 
 * @param process the current Coordinator-Splitter and Merger-Reporter process
 * @param process_data the data of the current Coordinator-Splitter and Merger-Reporter process
 */
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
        process->parent_to_child_fd[i] = (int*)malloc(sizeof(int) * 2); 
        process->child_to_parent_fd[i] = (int*)malloc(sizeof(int) * 2); 
        
        if (process->parent_to_child_fd[i] == NULL || process->child_to_parent_fd[i] == NULL) { // Check for any errors
            perror("Memory Error"); 
            exit(1); 
        }
    }
}

/**
 * @brief Deletes the memory used for the Coordinator-Splitter and Merger-Reporter process
 * @param process the current Coordinator-Splitter and Merger-Reporter process
 */
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

/**
 * @brief Runs the Coordinator-Splitter and Merger-Reporter process
 * @param process the current Coordinator-Splitter and Merger-Reporter process
 */
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
        else if (process->childProcessesIds[i] == 0) {
            runChildProcess(process, i); // Run the current child process
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
    // and send the appropriate data to the child processes
    unsigned int** recordsRangeToSort = getRecordsRangeArray(process);

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

    // Initialize some supporting arrays to read the data sent from the child processes
    Record** sortedRecords = (Record**)malloc(sizeof(Record*) * process->numberofChildProcesses);
    unsigned int* recordsCounts = (unsigned int*)malloc(sizeof(unsigned int) * process->numberofChildProcesses);

    unsigned int recordsCount;
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++) 
    {
        recordsCount = (recordsRangeToSort[i][1] - recordsRangeToSort[i][0] + 1);
        sortedRecords[i] = (Record*)malloc(sizeof(Record) * recordsCount);
        recordsCounts[i] = recordsCount;
    }

    // Reading the sorted records the work-spliter and result-merger has sent back to the cooridinator-spliter
    for (unsigned int i = 0; i < process->numberofChildProcesses; i++)
    {
        recordsCount = recordsCounts[i];

        // Reading every record from the current child process
        for (unsigned int j = 0; j < recordsCount; j++) {
            Record currentRecord;
            read(process->child_to_parent_fd[i][READ_END], &currentRecord, sizeof(Record));

            sortedRecords[i][j] = currentRecord;
        }
    }

    // Merge the fetched records into a single array and print it to stdout
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

/**
 * @brief Represents the Coordinator-Splitter and Merger-Reporter process
 * @param process the current Coordinator-Splitter and Merger-Reporter process
 */
void CSMR_print(CSMR_process* process)
{
    printf("\nHello from the Coordinator-Spliter and Merger-Reporter %d with:\n", process->processId);

    printf("- Input File Name: %s\n", process->inputFileName);
    printf("- Number of records in the file: %d\n", process->numberOfRecords);

    printf("- Number of child processes: %d\n", process->numberofChildProcesses);

    printf("- Sorting Algorithm 1: %s\n", process->sortingAlgorithm1);
    printf("- Sorting Algorithm 2: %s\n\n", process->sortingAlgorithm2);
}
