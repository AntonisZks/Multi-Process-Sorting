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

/**
 * @brief Calculates whether a record is lower that another, according to their last name, first name and custid
 * 
 * @param record1 the first record
 * @param record2 the second record
 * @return true or false whether record1 is lower that record2
 */
boolean isLowerThan(const Record record1, const Record record2)
{
    int lastNameResult = strcmp(record1.lastName, record2.lastName);

    if (lastNameResult < 0) {
        return TRUE;
    
    } else if (lastNameResult == 0) {
        
        int firstNameResult = strcmp(record1.firstName, record2.firstName);

        if (firstNameResult < 0) {
            return TRUE;
        
        } else if (firstNameResult == 0) {
            return record1.custid < record2.custid;
        }
    }

    return FALSE;
}

/**
 * @brief Prints the contents of a record (First name, Last name, custid and postcode)
 * @param record the record to be printed
 */
void printRecord(const Record record) {
    printf("%s %s %d %s\n", record.lastName, record.firstName, record.custid, record.postCode);
}

#endif /* RECORD_H */
