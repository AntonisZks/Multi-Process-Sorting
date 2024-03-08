#include <stdio.h>
#include <stdlib.h>
#include "../../include/record.h"

static Record* getRecords(char** argv);
static void mergeSort(Record array[], unsigned int leftIndex, unsigned int rightIndex);
static void merge(Record array[], unsigned int leftIndex, unsigned int middleIndex, unsigned int rightIndex);

/**
 * @brief Program that sorts a specific amount of records, inside a file using MergeSort Algorithm
 * 
 * @param argc the number of arguments
 * @param argv the arguments of the program (filename, totalRecords, start index, end index)
 * @return success or failure code
 * 
 * @author Antonis Zikas
 * @since 08/03/2024
 */
int main(int argc, char* argv[])
{
    Record* records = getRecords(argv);
    unsigned int recordsCounter = atoi(argv[4]) - atoi(argv[3]) + 1;

    //printf("Size: %d\n", atoi(argv[4]) - atoi(argv[3])+1);
    mergeSort(records, 0, recordsCounter - 1);

    for (unsigned int i = 0; i < recordsCounter; i++) {
        printf("%d) ", i + 1); printRecord(records[i]);
    }

    free(records);

    return 0;
}

/**
 * @brief Sorts the given array using MergeSort Algorithm
 * 
 * @param array the array to sort
 * @param leftIndex initial to 0
 * @param rightIndex initial to the size of the array - 1
 */
void mergeSort(Record array[], unsigned int leftIndex, unsigned int rightIndex)
{   
    //printf("Middle: %d\n", (leftIndex + rightIndex) / 2);
    if (leftIndex < rightIndex) {
        unsigned int middleIndex = (leftIndex + rightIndex) / 2; // Calculate the middle index of the array for next calls

        mergeSort(array, leftIndex, middleIndex);      // Call Merge Sort for the left sub-array
        mergeSort(array, middleIndex + 1, rightIndex); // Call Merge Sort for the right sub-array

        merge(array, leftIndex, middleIndex, rightIndex); // Merge the results
    }
}

/**
 * @brief Supporting function for the MergeSort Algorithm. Merges the results
 * 
 * @param array the array to sort
 * @param leftIndex initial to 0
 * @param middleIndex initial to the middle index of the array
 * @param rightIndex initial to the size of the array - 1
 */
void merge(Record array[], unsigned int leftIndex, unsigned int middleIndex, unsigned int rightIndex)
{
    unsigned int leftArraySize = middleIndex - leftIndex + 1;
    unsigned int rightArraySize = rightIndex - middleIndex;

    Record leftArray[leftArraySize], rightArray[rightArraySize]; // Create temporary arrays 

    // Copying data to temporary arrays
    for (unsigned int i = 0; i < leftArraySize; i++) {
        leftArray[i] = array[leftIndex + i];
    }
    for (unsigned int j = 0; j < rightArraySize; j++) {
        rightArray[j] = array[middleIndex + j + 1];
    }

    // Merge the temporary arrays back to the original array
    unsigned int leftArrayIndex = 0, rightArrayIndex = 0, mergedArrayIndex = leftIndex;
    
    while (leftArrayIndex < leftArraySize && rightArrayIndex < rightArraySize)
    {   
        if (isLowerThan(leftArray[leftArrayIndex], rightArray[rightArrayIndex])) {
            array[mergedArrayIndex++] = leftArray[leftArrayIndex++];
        }
        else {
            array[mergedArrayIndex++] = rightArray[rightArrayIndex++];
        }
    }

    // Copy the remaining elements of the left temporary array to the original one, if there are any
    while (leftArrayIndex < leftArraySize) {
        array[mergedArrayIndex++] = leftArray[leftArrayIndex++]; 
    }

    // Copy the remaining elements of the right temporary array to the original one, if there are any
    while (rightArrayIndex < rightArraySize) {
        array[mergedArrayIndex++] = rightArray[rightArrayIndex++];
    }
}

/**
 * @brief Returns an array of the specific records passed, in order to sort
 * 
 * @param arguments the command line arguments of the program
 * @return an array of records
 */
static Record* getRecords(char** arguments)
{
    unsigned int totalRecords = atoi(arguments[2]), startIndex = atoi(arguments[3]), endIndex = atoi(arguments[4]);
    char* filename = arguments[1];
    unsigned int counter = 0;
    Record currentRecord;

    // Create the array of records, to sort
    Record* records = (Record*)malloc(sizeof(Record) * (endIndex - startIndex + 1));
    if (records == NULL) {
        perror("Memory Error");
        exit(1);
    }

    // Open the input file to get the records
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("File Error");
        exit(1);
    }

    // Storing the records
    for (unsigned int i = 0; i < totalRecords; i++)
    {
        fread(&currentRecord, sizeof(Record), 1, fp);
        
        if (i == endIndex + 1)    break;
        else if (i >= startIndex) records[counter++] = currentRecord;
    }

    return records;
}
