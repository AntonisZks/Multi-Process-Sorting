/* Filename: main.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/coordinatorSpliterMergerReporter.h"

#define TRUE  (1)
#define FALSE (0)

typedef unsigned int boolean;

static void printApplicationUsage(const char* appName);
static boolean getCommandLineArguments(int argc, char** argv, char** file, unsigned int* processes, char** sorting1, char** sorting2);

/**
 * @brief Main Function of the Application
 *
 * @param argc the number of the command arguments
 * @param argv the actual command line arguments of the program
 * @return success code or error code
 *
 * @author Antonis Zikas
 * @since 03/03/2024
 */
int main(int argc, char* argv[])
{
    // Define the application parameters
    unsigned int numberOfChildren;
    char *dataFileName, *sortingAlgorithm1, *sortingAlgorithm2;

    // Get the command line arguments from the user
    if (!getCommandLineArguments(argc, argv, &dataFileName, &numberOfChildren, &sortingAlgorithm1, &sortingAlgorithm2)) {
        return 1;
    }

    // Construct and initialize the Coordinator-Spliter and Merger-Reporter (Root Process)
    CSMR_process rootProcess;
    CSMR_data rootData = { numberOfChildren, dataFileName, sortingAlgorithm1, sortingAlgorithm2 };

    CSMR_init(&rootProcess, &rootData);
    CSMR_run(&rootProcess);

    CSMR_delete(&rootProcess);

    return 0;
}

/**
 * @brief Prints the syntax of the application's execution
 * @param appName the name of the executable file
 */
static void printApplicationUsage(const char* appName) {
    printf("Usage: %s -i <DataFile> -k <NumofChildren> -e1 <sorting1> -e2 <sorting2>\n", appName);
}

/**
 * @brief Receives the command line arguments, the user has passed during the execution of the program
 *
 * @param argc the number of arguments
 * @param argv the actual arguments
 * @param file the filename of the data
 * @param processes the number of child processes the root is going to have
 * @param sorting1 the first sorting algorithm
 * @param sorting2 the second sorting algorithm
 * @return true is everything worked fine, false otherwise
 */
static boolean getCommandLineArguments(int argc, char** argv, char** file, unsigned int* processes, char** sorting1, char** sorting2)
{
    // Checking if the number of command arguments is valid
    if (argc != 9) {
        printApplicationUsage(argv[0]);
        return FALSE;
    }

    // Otherwise receive all the arguments from the command line
    for (unsigned int i = 1; i < argc; i += 2)
    {
        char* flag = argv[i];
        char* value = argv[i + 1];

        // Determine the flag of the current argument and receive its value
        if      (strcmp(flag, "-i")  == 0) { *file = value; }
        else if (strcmp(flag, "-k")  == 0) { *processes = atoi(value); }
        else if (strcmp(flag, "-e1") == 0) { *sorting1 = value; }
        else if (strcmp(flag, "-e2") == 0) { *sorting2 = value; }
    }

    return TRUE;
}
