#include "../../include/record.h"

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
