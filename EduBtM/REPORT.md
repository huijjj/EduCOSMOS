# EduBtM Report

Name: 정희종

Student id: 20190846

# Problem Analysis

In this project we have to implement API functions and internal functions used by API funtions to manage indexing B tree structure. Key and ObjectId of an Object are maintained sorted at the leaf page of the B tree.


1. BtreeLeaf

Page data structure representing a leaf node of a B+ tree index. Consists of Header and Data region.

2. BtM_LeafEntry

Data structure representing a leaf index entry stored in a leaf page of a B+ tree index.

3. BtreeInternal

The page data structure representing an internal node of a B+ tree index. Also consists of Header and Data region.

4. Btm_InternalEntry

Data structure representing an internal index entry stored in an internal page of a B+ tree index.

5. KeyValue

Data structure to store a key and related information used in a B+ tree index. Able to store variable length keys.

6. KeyDesc

Data structure to store information for separating individual attribute values in a sequence of attribute values of a key.

7. BtreeCursor

Data structure of the cursor pointing to a leaf index entry of the B+ tree index. 
Used for storing the information about the current leaf index entry being searched and the next leaf index entry to be searched when sequentially searching for objects satisfying the search condition. 
In short, it is cursor used for traversing the B tree. 

# Design For Problem Solving

## High Level

1. EduBtM_CreateIndex()

Create a new B+ tree index in an index file, and return the page ID of the root page of index created.

2. EduBtM_DropIndex()

Delete a B+ tree index from an index file. 

3. EduBtM_InsertObject()

Insert a new object into a B+ tree index. 

4. EduBtM_DeleteObject() 

Delete an object from a B+ tree index.

5. EduBtM_Fetch()

Search for the first object satisfying the search condition in the B+ tree index, and return the cursor pointing to the object found. 

6. EduBtM_FetchNext()

Fetches the object next to the current object satisfying the search condition (considering only the stop key value but not start key value) in the B+ tree index, and return the cursor pointing to the object found.

There are to many internal functions to explain, thus I decided to give up. But however, all internal function does just as their name says. Implementations are trivial.

## Low Level

1. EduBtM_CreateIndex()

Allocate the first page of index file by calling btm_AllocPage() and initailize it as the root page. Return the page ID of initailized page.

2. EduBtM_DropIndex()

Deallocate a root page and its all descendant. This can be done by calling edubtm_FreePages with root as a parameter.

3. EduBtM_InsertObject()

Call edubtm_Insert() to insert the (key, oid) pair for the new object into the B+ tree index. If split occured in root during insertion, call edubtm_root_insert() to make a new root and maintain tree structure.

4. EduBtM_DeleteObject() 

Call edubtm_Delete() to delete the (key, oid) pair for the object to be deleted from the B+ tree index. 
If underflow has occurred in the root page, call btm_root_delete() to handle it.
If the root page has been split, call edubtm_root_insert() to handle it.

5. EduBtM_Fetch()

If startCompOp given as a parameter is SM_BOF, search for the first object in the B+ tree index by calling edubtm_FirstObject(). 
If startCompOp given as a parameter is SM_EOF, search for the last object in the B+ tree index by calling edubtm_LastObject(). 
Other cases, call edubtm_Fetch() to search for a leaf index entry containing the first entry satisfying the search condition in the B+ tree index. 
Return the cursor pointing to the leaf index entry found. 

6. EduBtM_FetchNext()

Call edubtm_FetchNext() to search for the leaf index entry next to the current leaf index entry satisfying the search condition in the B+ tree index. 
Return the cursor pointing to the leaf index entry found. 

Again, I will skip the explanation for internal functions.

# Mapping Between Implementation And the Design

Please refer to the code for detailed information. However details do not differ a lot from the design above, only error checks are added.