/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module : EduOM_DestroyObject.c
 * 
 * Description : 
 *  EduOM_DestroyObject() destroys the specified object.
 *
 * Exports:
 *  Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 */

#include "EduOM_common.h"
#include "Util.h"		/* to get Pool */
#include "RDsM.h"
#include "BfM.h"		/* for the buffer manager call */
#include "LOT.h"		/* for the large object manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_DestroyObject()
 *================================*/
/*
 * Function: Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 * 
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  (1) What to do?
 *  EduOM_DestroyObject() destroys the specified object. The specified object
 *  will be removed from the slotted page. The freed space is not merged
 *  to make the contiguous space; it is done when it is needed.
 *  The page's membership to 'availSpaceList' may be changed.
 *  If the destroyed object is the only object in the page, then deallocate
 *  the page.
 *
 *  (2) How to do?
 *  a. Read in the slotted page
 *  b. Remove this page from the 'availSpaceList'
 *  c. Delete the object from the page
 *  d. Update the control information: 'unused', 'freeStart', 'slot offset'
 *  e. IF no more object in this page THEN
 *	   Remove this page from the filemap List
 *	   Dealloate this page
 *    ELSE
 *	   Put this page into the proper 'availSpaceList'
 *    ENDIF
 * f. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    eBADFILEID_OM
 *    some errors caused by function calls
 */
Four EduOM_DestroyObject(
    ObjectID *catObjForFile,	/* IN file containing the object */
    ObjectID *oid,		/* IN object to destroy */
    Pool     *dlPool,		/* INOUT pool of dealloc list elements */
    DeallocListElem *dlHead)	/* INOUT head of dealloc list */
{
    Four        e;		/* error number */
    Two         i;		/* temporary variable */
    FileID      fid;		/* ID of file where the object was placed */
    PageID	pid;		/* page on which the object resides */
    SlottedPage *apage;		/* pointer to the buffer holding the page */
    Four        offset;		/* start offset of object in data area */
    Object      *obj;		/* points to the object in data area */
    Four        alignedLen;	/* aligned length of object */
    Boolean     last;		/* indicates the object is the last one */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */
    DeallocListElem *dlElem;	/* pointer to element of dealloc list */
    PhysicalFileID pFid;	/* physical ID of file */
    
    

    /*@ Check parameters. */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

    if (oid == NULL) ERR(eBADOBJECTID_OM);

    // get page
    MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
    e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
    if(e) {
        ERR(e);
    }

    // remove page from available list
    e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
    if(e) {
        ERR(e);
    }

    // get object
    offset = apage->slot[-(oid->slotNo)].offset;
    obj = apage->data + offset;
    alignedLen = ALIGNED_LENGTH(obj->header.length);

    // clear slot
    apage->slot[-1 * (oid->slotNo)].offset = EMPTYSLOT;

    // update header if needed
    last = FALSE;
    i = 0;
    while(i < apage->header.nSlots) {
        if(apage->slot[-i].offset != EMPTYSLOT) {
            break;
        }
        i++;
    }
    if(i == apage->header.nSlots) {
        last = TRUE;
    }

    if(last) { // if object is only object in this page
        // get catalog file
        e = BfM_GetTrain(catObjForFile, &catPage, PAGE_BUF);
        if(e) {
            ERR(e);
        } 
        GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
        if(oid->pageNo == catEntry->firstPage) { // this page is first page of file
            // update header
            apage->header.nSlots = 1;
            apage->header.free = 0;
            apage->header.unused = 0;
            BfM_SetDirty(&pid, PAGE_BUF);

            e = om_PutInAvailSpaceList(catObjForFile, &pid, apage); // insert into available list
            if(e) {
                BfM_FreeTrain(catPage, PAGE_BUF);
                BfM_FreeTrain(apage, PAGE_BUF);
                ERR(e);
            }
        }
        else { // this page is not first page of file
            e = om_FileMapDeletePage(catObjForFile, apage); // delete page from file page list
            if(e) {
                BfM_FreeTrain(catPage, PAGE_BUF);
                BfM_FreeTrain(apage, PAGE_BUF);
                ERR(e);
            }

            // add page to deallocation list
            dlElem = dlHead;
            e = Util_getElementFromPool(dlPool, dlElem);
            if(e) {
                BfM_FreeTrain(catPage, PAGE_BUF);
                BfM_FreeTrain(apage, PAGE_BUF);
                ERR(e);
            }
            dlElem->type = DL_PAGE;
            dlElem->next = dlHead;
            dlElem->elem.pid = pid;
        }
        e = BfM_FreeTrain(catPage, PAGE_BUF);
        if(e) {
            BfM_FreeTrain(apage, PAGE_BUF);
            ERR(e);
        }
    }
    else { // if this object is not the only object
        if(oid->slotNo + 1 == apage->header.nSlots) { // if object is in the last slot
            apage->header.nSlots--;
        }
        if(offset + alignedLen + sizeof(ObjectHdr) == apage->header.free) { // if object is the last object in data region
            apage->header.free -= sizeof(ObjectHdr);
            apage->header.free -= alignedLen;
        }
        else {
            apage->header.unused += sizeof(ObjectHdr);
            apage->header.unused += alignedLen;
        }
        e = om_PutInAvailSpaceList(catObjForFile, &pid, apage); // insert to available list
        if(e) {
            BfM_FreeTrain(apage, PAGE_BUF);
            ERR(e);
        }
        BfM_SetDirty(&pid, PAGE_BUF);
    }
    
    e = BfM_FreeTrain(apage, PAGE_BUF);
    if(e) {
        ERR(e);
    }

    return(eNOERROR);
    
} /* EduOM_DestroyObject() */
