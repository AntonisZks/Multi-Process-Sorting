/* Filename: record.h */

#ifndef RECORD_H
#define RECORD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_BUFFER_SIZE 20
#define POSTCODE_BUFFER_SIZE 6

#define TRUE  (1)
#define FALSE (0)

typedef unsigned int boolean;

/**
 * @brief Structure that represents a Record of the application
 * @author Antonis Zikas
 * @since 04/03/2024
 */
typedef struct ApplicationRecord
{
    int custid;
    char firstName[NAME_BUFFER_SIZE];
    char lastName[NAME_BUFFER_SIZE];
    char postCode[POSTCODE_BUFFER_SIZE];

} Record;

boolean isLowerThan(const Record record1, const Record record2);
void printRecord(const Record record);

#endif /* RECORD_H */
