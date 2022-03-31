# EduOM Report

Name: 정희종

Student id: 20190846

# Problem Analysis

For EduOM project, we have to implement some API's for managing objects in slotted page.
Each data file is managed by sm_catOverlayForData structure. sm_catOverlayForData holds linked list of pages in the file. Each page is a slotted page. Slotted page consists of header for metadata and slot for object indexing and data region for actually storing the objects. Each object again consists of header for storing metadata and data region for storing actuall data.

Our goal is to implement APIs for creating(CreateObject) and destroying(DestroyObject) objects as well as reading(ReadObject), searching(PrevObject, NextObject) and managing(CompactPage) objects. Please refer to contents below for details of each function.

# Design For Problem Solving

## High Level

1. EduOM_CreateObject(eduom_createobject)
    Insert a new object into the page identical(or adjacent) to the page where near object given as parameter resides. Return the id of inserted object. Every functionality is implemented in eduom_createobject and EduOM_CreateObject is just a wrapup function for simple error checking.

2. EduOM_CompactPage
    Rearrange objects in the page to increase contigous free region.

3. EduOM_DestroyObject
    Destroy object in the page and update page header and slots if needed. If needed(if deleted page was only object in the page and the page is not the first page of a file) update the linked list structure. And move page to proper available space list.

4. EduOM_ReadObject
    Read object in to given buffer data using start offset and length which are given as parameters. 

5. EduOM_NextObject
    Get the next object of the given object. If given object is the last object in the page, get the first object of the next page. If given object is the last object of last page return End-Of-Search.

6. EduOM_PrevObject
    Get the previous object of the given object. If given object is the first object in the page, get the last object of the previous page. If given object is the first object of the first page return End-Of-Search.

## Low Level

1. EduOM_CreateObject(eduom_createobject)
    Calculate the needed space for object. Get the page of given near object, if near object is not given get the page from available list according to the needed space. If chosen page has enough free space, check if contigous space is enough for object to fit in, if contigous space is not enough compact page by calling EduOM_CompactPage. If space is not enough, allocate a new page and intialize the header and add to file linked list. Then insert the object and update the slottedpage header and slots. Finally add page to proper available list. And before free-ing the page, set the dirty bit to indicate that new object is added to page and the page has been modified.

2. EduOM_CompactPage
    Copy page to temporary page. And for each object in the slot, rearrange objects starting from offset 0 to compact the page. Note that there only offset information is updated in slot array and nSlots remains unchanged. And update the start of free region and set unused byte to 0. And set dirty bit since page has been modified.

3. EduOM_DestroyObject
    Get the object metadata(offset, length) in page. Clear slot corresponding to given slotNumber(slotNo). If given slot was the last slot reduce nSlots by 1. And if the deleted object was the only object in the page and page is not the first page of a file, delete page using om_FileMapeDeletePage and add page to deallocation list. If object is not the only object, update the page header. And set dirty bit since page is modified.

4. EduOM_ReadObject
    Get the object corresponding to given parameters. Copy data according to start offset within the object data and length. Note that start offset is not the offset from start of a page data region but is from the start of an object data region.

5. EduOM_NextObject
    Get the page of current object given as curOID. If curOID is not given get the first page of the file. Search the slot array from the next index of current object(or from the start if not given). If next object is not in the same page, get the next page and search the slot array from the begining. If the given obejct was the last object of a file return End-Of-Search. If next object is chosen return the OID and header of chosen object.

6. EduOM_PrevObject
    Same as EduOM_NextObject except that first is changed into last and next is changed into previous. Please refer to explanation above.

# Mapping Between Implementation And the Design

Implementation details are included in comments in the code, please refer to code and comments for detailed information. However details do not differ a lot from the design above, only error checks are added.
