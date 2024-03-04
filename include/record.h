/* Filename: record.h */

#ifndef RECORD_H
#define RECORD_H

#define NAME_BUFFER_SIZE 20
#define POSTCODE_BUFFER_SIZE 6

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

#endif /* RECORD_H */
