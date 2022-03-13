# EduBfM Report

Name: 정희종

Student id: 20190846

# Problem Analysis

Implement internal functions to manage internal buffer structure(bufInfo, bufTable, buffer pool).
And Implement EduBfM's API using implemented internal funcitons.


1. bufInfo 

Structure maintaining information of 2 types of buffer structures. 
One for PAGE_BUF and other for LOT_LEAF_BUF. Each element of this structure holds other data structure(bufTable, bufferPool, hashTable) of according index(0 for page buffer, 1 for train buffer).

2. bufTable

Structure holding the metadata of each buffer element. Each element of bufTable holds the metadata for page(or train) in the same index of bufferPool. By modifying data in bufTable we can update the state of page(or train) in buffer. Internal functions are mostly about handling this data.

3. bufferPool

Structure holding the actual cached data. Page(or train) read from the disk is stored as an element of bufferPool.

4. hashTable

Structure holding indices of hashed page. We can obtain certain page's hashvalue using page number and volumn number. Combination of page number and volumn number is unique, but due to structural limitation(memory is smaller than disk) hash collision is possible. Hash collision is dealt with chaining method in this project.


And again, our goal in this project is to implement functions managing this structures and provide APIs functioning like pre-defined behavior upon our very own implementation.

# Design For Problem Solving

## High Level

1. edubfm_ReadTrain

    Read page/train from disk into buffer element.
    
2. edubfm_AllocTrain

    Allocate buffer element in bufferPool to store page/train.

3. edubfm_Insert

    Insert information of buffer element to hashTable. Use chaining in case of collision.

4. edubfm_Delete

    Delete page/train's information from hashTable. Update chaining if needed.

5. edubfm_Deleteall

    Delete every entry in hashTable. Both page and train hashTable.

6. edubfm_LookUp

    Find buffer index corresponding to given key.

7. edubfm_FlushTrain

    Flush changes of page/train to disk if needed.

8. EduBfM_GetTrain

    Fix page/train in bufferPool, and get the pointer to fixed element.
    First check if page/train is already in bufferPool if so, fix that page and return the pointer.
    Else, allocate buffer element and get page/train to bufferPool, and do the same thing as above.

9. EduBfM_FreeTrain

    Unfix page/train in bufferPool.

10. EduBfM_SetDirty

    Update the dirty bit of page/train to indicate that this page is modified.

11. EduBfM_FlushAll

    Flush changes in memory(buffer) to disk.

12. EduBfM_DiscardAll

    Discard all pages and trains in buffer. Do not flush changes to disk.

## Low Level

1. edubfm_ReadTrain

    Read page/train into given memory location using provided disk function RDsM_ReadTrain.
    
2. edubfm_AllocTrain

    Starting from next victim index stored in bufInfo, loop around bufTable to find victim page using refer bit(second chance algorithm). After victim page/train is chosen check if page/train is dirty. If so flush changes to disk by edubfm_FlushTrain. And remove victim page's information from hashTable. Note that victim page/train might be empty buffer element, this can be known by checking page/train's hashKey. After victim is evicted(of course we leave bufferPool element unchanged since we already no that we don't use that element by checking bufTable), update bufTable to indicate that element is empty. And finally return the index of evicted victim so that the new page/train can use this empty-ed out element.  

3. edubfm_Insert

    Get the hashValue of given key and check if entry corresponding to the hashValue is empty. If not, update the next hash entry field of given page/train with original hash table entry. Update hashTable entry with given index.

4. edubfm_Delete

    Delete corresponding hashTable entry from hashTable. If given entry has to be accessed by chaining, update the linked list so that linked list is maintained after delete.

5. edubfm_Deleteall

    Delete all entries in hashTable, both page and train. This is basically just updating every entry to NIL.

6. edubfm_LookUp

    Find the index corresponding to given key by checking hashTable. hashTable is indexed by hashValue. hashValue can be obtained from given key.

7. edubfm_FlushTrain

    Find index of given page/train with edubfm_LookUp. Check if page/train is dirty. If so, flush changes to disk with provided disk function RDsM_WriteTrain and update dirty bit to 0.

8. EduBfM_GetTrain

    Call edubfm_LookUp to check if page/train is already inside the bufferPool.
    If so increase the fix count of that page/train in bufTable.
    Else, allocate new buffer element with edubfm_AllocTrain and read data into allocated buffer element with edubfm_ReadTrain. And as above, update the fix count.
    And update the value at retBuf to pointer to buffer. 

9. EduBfM_FreeTrain

    Call edubfm_LookUp to find corresponding page. If page is not found, throw an error.
    If page is found, decrease the fix count. If fix count is already 0 print out a message indicating that this page is not fixed by any transaction.

10. EduBfM_SetDirty

    Call edubfm_LookUp to find corresponding page. If page is not found, throw an error.
    If page is found, update the dirty bit in bufTable with logical OR operation.

11. EduBfM_FlushAll

    Loop through every pages and trains in buffer. Check if page/train is dirty by checking it's dirty bit.
    If page/train in interest is dirty while looping through, call edubfm_FlushTrain to flush change to disk.

12. EduBfM_DiscardAll

    Loop through bufferTables every pages and trains. Initialize every entries in bufferTable(set hashkey to nil, bits to ALL_0, fixed count to 0). And delete all hashTable entries by calling edubfm_DeleteAll.

# Mapping Between Implementation And the Design

Implementation details are included in comments, please refer to code comments for detailed information.
However details do not differ a lot from the design above, only error checks are added.