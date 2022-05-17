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
 * Module: edubtm_Split.c
 *
 * Description : 
 *  This file has three functions about 'split'.
 *  'edubtm_SplitInternal(...) and edubtm_SplitLeaf(...) insert the given item
 *  after spliting, and return 'ritem' which should be inserted into the
 *  parent page.
 *
 * Exports:
 *  Four edubtm_SplitInternal(ObjectID*, BtreeInternal*, Two, InternalItem*, InternalItem*)
 *  Four edubtm_SplitLeaf(ObjectID*, PageID*, BtreeLeaf*, Two, LeafItem*, InternalItem*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_SplitInternal()
 *================================*/
/*
 * Function: Four edubtm_SplitInternal(ObjectID*, BtreeInternal*,Two, InternalItem*, InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  At first, the function edubtm_SplitInternal(...) allocates a new internal page
 *  and initialize it.  Secondly, all items in the given page and the given
 *  'item' are divided by halves and stored to the two pages.  By spliting,
 *  the new internal item should be inserted into their parent and the item will
 *  be returned by 'ritem'.
 *
 *  A temporary page is used because it is difficult to use the given page
 *  directly and the temporary page will be copied to the given page later.
 *
 * Returns:
 *  error code
 *    some errors caused by function calls
 *
 * Note:
 *  The caller should call BfM_SetDirty() for 'fpage'.
 */
Four edubtm_SplitInternal(
    ObjectID                    *catObjForFile,         /* IN catalog object of B+ tree file */
    BtreeInternal               *fpage,                 /* INOUT the page which will be splitted */
    Two                         high,                   /* IN slot No. for the given 'item' */
    InternalItem                *item,                  /* IN the item which will be inserted */
    InternalItem                *ritem)                 /* OUT the item which will be returned by spliting */
{
    Four                        e;                      /* error number */
    Two                         i;                      /* slot No. in the given page, fpage */
    Two                         j;                      /* slot No. in the splitted pages */
    Two                         k;                      /* slot No. in the new page */
    Two                         maxLoop;                /* # of max loops; # of slots in fpage + 1 */
    Four                        sum;                    /* the size of a filled area */
    Boolean                     flag=FALSE;             /* TRUE if 'item' become a member of fpage */
    PageID                      newPid;                 /* for a New Allocated Page */
    BtreeInternal               *npage;                 /* a page pointer for the new allocated page */
    Two                         fEntryOffset;           /* starting offset of an entry in fpage */
    Two                         nEntryOffset;           /* starting offset of an entry in npage */
    Two                         entryLen;               /* length of an entry */
    btm_InternalEntry           *fEntry;                /* internal entry in the given page, fpage */
    btm_InternalEntry           *nEntry;                /* internal entry in the new page, npage*/
    Boolean                     isTmp;

	BtreeInternal tpage;

    memcpy(&tpage, fpage, PAGESIZE);

	j = 0;
    k = 0;
	sum = 0;
	fEntryOffset = 0;
    nEntryOffset = 0;
	if(e = btm_AllocPage(catObjForFile, &fpage->hdr.pid, &newPid)) {
        ERR(e);
    }
	if(e = edubtm_InitInternal(&newPid, FALSE, FALSE)) {
        ERR(e);
    }
	if(e = BfM_GetNewTrain(&newPid, &npage, PAGE_BUF)) {
        ERR(e);
    }

	maxLoop = fpage->hdr.nSlots + 1; 
    high += 1;
	for(i = 0; i < maxLoop; i++) {
		if(sum < BI_HALF) {
			if(i < high) {
				fEntry = &(tpage.data[tpage.slot[-1 * i]]);
				entryLen = sizeof(ShortPageID) + 2 + fEntry->klen;
				
                memcpy(&(fpage->data[fEntryOffset]), &(tpage.data[tpage.slot[-1 * i]]), entryLen);
			}
			else if(i == high) {
				entryLen = sizeof(ShortPageID) + 2 + ALIGNED_LENGTH(item->klen);
				
                memcpy(&(fpage->data[fEntryOffset]), item, entryLen);
			}
			else {
				fEntry = &(tpage.data[tpage.slot[-1 * (i - 1)]]);
				entryLen = sizeof(ShortPageID) + 2 + fEntry->klen;

				memcpy(&(fpage->data[fEntryOffset]), &(tpage.data[tpage.slot[-1 * (i - 1)]]), entryLen);
			}

			fpage->slot[-1 * j] = fEntryOffset;
			fEntryOffset += entryLen;
			j++;
		}
		else {
			if(i < high) {
				nEntry = &(tpage.data[tpage.slot[-1 * i]]);
				entryLen = sizeof(ShortPageID) + 2 + nEntry->klen;

				memcpy(&(npage->data[nEntryOffset]), &(tpage.data[tpage.slot[-1*i]]), entryLen);

				if(tpage.slot[-1 * i] + entryLen == fpage->hdr.free) {
					fpage->hdr.free -= entryLen;
                }
				else {
					fpage->hdr.unused += entryLen;
                }
			}
			else if(i == high) {
				entryLen = sizeof(ShortPageID) + 2 + item->klen;

				memcpy(&(npage->data[nEntryOffset]), item, entryLen);
			}
			else {
				nEntry = &(tpage.data[tpage.slot[-1 * (i - 1)]]);
				entryLen = sizeof(ShortPageID) + 2 + nEntry->klen;

				memcpy(&(npage->data[nEntryOffset]), &(tpage.data[tpage.slot[-1 * (i - 1)]]), entryLen);
				
                if(tpage.slot[-1 * (i - 1)] + entryLen == fpage->hdr.free) {
					fpage->hdr.free -= entryLen;
                }
				else {
					fpage->hdr.unused += entryLen;
                }
			}

			npage->slot[-1 * k] = nEntryOffset;
			nEntryOffset += entryLen;
			k++;
		}

		sum += entryLen + 2;
	}

	nEntry = &(npage->data[npage->slot[0]]);
	npage->hdr.p0 = nEntry->spid;
    npage->hdr.free = nEntryOffset;
	npage->hdr.nSlots = k;
	
    fpage->hdr.nSlots = j;
	memcpy(ritem, nEntry, sizeof(InternalItem));
	
	if(fpage->hdr.type & ROOT) {
		fpage->hdr.type -= ROOT;
	}

	if(e = BfM_SetDirty(&newPid, PAGE_BUF)) {
        BfM_FreeTrain(&newPid, PAGE_BUF);
        ERR(e);
    }
	if(e = BfM_SetDirty(&fpage->hdr.pid, PAGE_BUF)) {
        BfM_FreeTrain(&newPid, PAGE_BUF);
        ERR(e);
    }
    if(e = BfM_FreeTrain(&newPid, PAGE_BUF)) {
        ERR(e);
    }

    return(eNOERROR);
    
} /* edubtm_SplitInternal() */



/*@================================
 * edubtm_SplitLeaf()
 *================================*/
/*
 * Function: Four edubtm_SplitLeaf(ObjectID*, PageID*, BtreeLeaf*, Two, LeafItem*, InternalItem*)
 *
 * Description: 
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  The function edubtm_SplitLeaf(...) is similar to edubtm_SplitInternal(...) except
 *  that the entry of a leaf differs from the entry of an internal and the first
 *  key value of a new page is used to make an internal item of their parent.
 *  Internal pages do not maintain the linked list, but leaves do it, so links
 *  are properly updated.
 *
 *  Error code
 *  eDUPLICATEDOBJECTID_BTM
 *    some errors caused by function calls
 *
 * Note:
 *  The caller should call BfM_SetDirty() for 'fpage'.
 */
Four edubtm_SplitLeaf(
    ObjectID                    *catObjForFile, /* IN catalog object of B+ tree file */
    PageID                      *root,          /* IN PageID for the given page, 'fpage' */
    BtreeLeaf                   *fpage,         /* INOUT the page which will be splitted */
    Two                         high,           /* IN slotNo for the given 'item' */
    LeafItem                    *item,          /* IN the item which will be inserted */
    InternalItem                *ritem)         /* OUT the item which will be returned by spliting */
{
    Four                        e;              /* error number */
    Two                         i;              /* slot No. in the given page, fpage */
    Two                         j;              /* slot No. in the splitted pages */
    Two                         k;              /* slot No. in the new page */
    Two                         maxLoop;        /* # of max loops; # of slots in fpage + 1 */
    Four                        sum;            /* the size of a filled area */
    PageID                      newPid;         /* for a New Allocated Page */
    PageID                      nextPid;        /* for maintaining doubly linked list */
    BtreeLeaf                   tpage;          /* a temporary page for the given page */
    BtreeLeaf                   *npage;         /* a page pointer for the new page */
    BtreeLeaf                   *mpage;         /* for doubly linked list */
    btm_LeafEntry               *itemEntry;     /* entry for the given 'item' */
    btm_LeafEntry               *fEntry;        /* an entry in the given page, 'fpage' */
    btm_LeafEntry               *nEntry;        /* an entry in the new page, 'npage' */
    ObjectID                    *iOidArray;     /* ObjectID array of 'itemEntry' */
    ObjectID                    *fOidArray;     /* ObjectID array of 'fEntry' */
    Two                         fEntryOffset;   /* starting offset of 'fEntry' */
    Two                         nEntryOffset;   /* starting offset of 'nEntry' */
    Two                         oidArrayNo;     /* element No in an ObjectID array */
    Two                         alignedKlen;    /* aligned length of the key length */
    Two                         itemEntryLen;   /* length of entry for item */
    Two                         entryLen;       /* entry length */
    Boolean                     flag;
    Boolean                     isTmp;
 
    memcpy(&tpage, fpage, PAGESIZE);
	
	if(e = btm_AllocPage(catObjForFile, &fpage->hdr.pid, &newPid)) {
        ERR(e);
    }
	if(e = edubtm_InitLeaf(&newPid, FALSE, FALSE)) {
        ERR(e);
    }
	if(e = BfM_GetNewTrain(&newPid, &npage, PAGE_BUF)) {
        ERR(e);
    }

	j = 0;
	k = 0;
	sum = 0;
    high += 1;
	flag = FALSE;
	fEntryOffset = 0;
	nEntryOffset = 0;
	maxLoop = fpage->hdr.nSlots + 1;
	for(i = 0; i < maxLoop; i++) {
		if(sum < BL_HALF) {
			if(i < high) {
				fEntry = &(tpage.data[tpage.slot[-1 * i]]);
				entryLen = 4 + ALIGNED_LENGTH(fEntry->klen) + sizeof(ObjectID);

				memcpy(&(fpage->data[fEntryOffset]), &(tpage.data[tpage.slot[-1 * i]]), entryLen);
			}
			else if(i == high) {
				fEntry = &(item->nObjects);
				entryLen = 4 + ALIGNED_LENGTH(item->klen);

				memcpy(&(fpage->data[fEntryOffset]), fEntry, entryLen);
				memcpy(&(fpage->data[fEntryOffset + entryLen]), &(item->oid), sizeof(ObjectID));
				
                entryLen += sizeof(ObjectID);
				flag = TRUE;
			}
			else {
				fEntry = &(tpage.data[tpage.slot[-1 * (i - 1)]]);
				entryLen = 4 + ALIGNED_LENGTH(fEntry->klen) + sizeof(ObjectID);

				memcpy(&(fpage->data[fEntryOffset]), &(tpage.data[tpage.slot[-1 * (i - 1)]]), entryLen);
			}

			fpage->slot[-1 * j] = fEntryOffset;
			
            fEntryOffset += entryLen;
			j++;
		}
		else {
			if(i < high) {
				nEntry = &(tpage.data[tpage.slot[-1 * i]]);
				entryLen = 4 + ALIGNED_LENGTH(nEntry->klen) + sizeof(ObjectID);
				
                memcpy(&(npage->data[nEntryOffset]), &(tpage.data[tpage.slot[-1 * i]]), entryLen);

				if(tpage.slot[-1 * i] + entryLen == fpage->hdr.free) {
					fpage->hdr.free -= entryLen;
                }
				else {
					fpage->hdr.unused += entryLen;
                }
			}
			else if(i == high) {
				nEntry = &(item->nObjects);
				entryLen = 4 + ALIGNED_LENGTH(item->klen);

				memcpy(&(npage->data[nEntryOffset]), nEntry, entryLen);
				memcpy(&(npage->data[nEntryOffset + entryLen]), &item->oid, sizeof(ObjectID));

				entryLen += sizeof(ObjectID);
			}
			else {
				nEntry = &(tpage.data[tpage.slot[-1 * (i - 1)]]);
				entryLen = 4 + ALIGNED_LENGTH(nEntry->klen) + sizeof(ObjectID);

				memcpy(&(npage->data[nEntryOffset]), &(tpage.data[tpage.slot[-1 * (i - 1)]]), entryLen);
				
                if (tpage.slot[-1 * (i - 1)] + entryLen == fpage->hdr.free) {
					fpage->hdr.free -= entryLen;
                }
				else {
					fpage->hdr.unused += entryLen;
                }
			}

			npage->slot[-1 * k] = nEntryOffset;
			
            nEntryOffset += entryLen;
			
            k++;
		}

		sum += entryLen + 2;
	}

	if(flag) {
		fpage->hdr.free += entryLen;
    }

	npage->hdr.nSlots = k;
    npage->hdr.free = nEntryOffset;
	npage->hdr.prevPage = fpage->hdr.pid.pageNo;
	npage->hdr.nextPage = fpage->hdr.nextPage;
	
	fpage->hdr.nSlots = j;
    fpage->hdr.nextPage = npage->hdr.pid.pageNo;
	
    if(npage->hdr.nextPage != NIL) {
        MAKE_PAGEID(nextPid, npage->hdr.pid.volNo, npage->hdr.nextPage);

        if(e = BfM_GetTrain(&nextPid, &mpage, PAGE_BUF)) {
            ERR(e);
        }
    
        mpage->hdr.prevPage = newPid.pageNo;

        if(e = BfM_SetDirty(&nextPid, PAGE_BUF)) {
            BfM_FreeTrain(&nextPid, PAGE_BUF);
            ERR(e);
        }
        if(e = BfM_FreeTrain(&nextPid, PAGE_BUF)) {
            ERR(e);
        }
    }

	nEntry = &(npage->data[npage->slot[0]]);
    ritem->spid = newPid.pageNo;
    ritem->klen = nEntry->klen;
    memcpy(ritem->kval, nEntry->kval, nEntry->klen);

	if(fpage->hdr.type & ROOT) {
		fpage->hdr.type -= ROOT;
	}

	if(e = BfM_SetDirty(&newPid, PAGE_BUF)) {
        BfM_FreeTrain(&newPid, PAGE_BUF);
        ERR(e);
    }
	if(e = BfM_SetDirty(&fpage->hdr.pid, PAGE_BUF)) {
        BfM_FreeTrain(&newPid, PAGE_BUF);
        ERR(e);
    }
	if(e = BfM_FreeTrain(&newPid, PAGE_BUF)) {
        ERR(e);
    }

	return eNOERROR;
    
} /* edubtm_SplitLeaf() */