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
 * Module: EduBtM_Fetch.c
 *
 * Description :
 *  Find the first object satisfying the given condition.
 *  If there is no such object, then return with 'flag' field of cursor set
 *  to CURSOR_EOS. If there is an object satisfying the condition, then cursor
 *  points to the object position in the B+ tree and the object identifier
 *  is returned via 'cursor' parameter.
 *  The condition is given with a key value and a comparison operator;
 *  the comparison operator is one among SM_BOF, SM_EOF, SM_EQ, SM_LT, SM_LE, SM_GT, SM_GE.
 *
 * Exports:
 *  Four EduBtM_Fetch(PageID*, KeyDesc*, KeyValue*, Four, KeyValue*, Four, BtreeCursor*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"


/*@ Internal Function Prototypes */
Four edubtm_Fetch(PageID*, KeyDesc*, KeyValue*, Four, KeyValue*, Four, BtreeCursor*);



/*@================================
 * EduBtM_Fetch()
 *================================*/
/*
 * Function: Four EduBtM_Fetch(PageID*, KeyDesc*, KeyVlaue*, Four, KeyValue*, Four, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the first object satisfying the given condition. See above for detail.
 *
 * Returns:
 *  error code
 *    eBADPARAMETER_BTM
 *    some errors caused by function calls
 *
 * Side effects:
 *  cursor  : The found ObjectID and its position in the Btree Leaf
 *            (it may indicate a ObjectID in an  overflow page).
 */
Four EduBtM_Fetch(
    PageID   *root,		/* IN The current root of the subtree */
    KeyDesc  *kdesc,		/* IN Btree key descriptor */
    KeyValue *startKval,	/* IN key value of start condition */
    Four     startCompOp,	/* IN comparison operator of start condition */
    KeyValue *stopKval,		/* IN key value of stop condition */
    Four     stopCompOp,	/* IN comparison operator of stop condition */
    BtreeCursor *cursor)	/* OUT Btree Cursor */
{
    int i;
    Four e;		   /* error number */

    
    if (root == NULL) ERR(eBADPARAMETER_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }
    
    if(startCompOp == SM_BOF) {
		if(e = edubtm_FirstObject(root, kdesc, stopKval, stopCompOp, cursor)) {
			ERR(e);
		}
	}
	else if(startCompOp == SM_EOF) {
		if(e = edubtm_LastObject(root, kdesc, stopKval, stopCompOp, cursor)) {
			ERR(e);
		}
	}
	else {
		if(e = edubtm_Fetch(root, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor)) {
			ERR(e);
		}
	}

    return eNOERROR;

} /* EduBtM_Fetch() */



/*@================================
 * edubtm_Fetch()
 *================================*/
/*
 * Function: Four edubtm_Fetch(PageID*, KeyDesc*, KeyVlaue*, Four, KeyValue*, Four, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the first object satisfying the given condition.
 *  This function handles only the following conditions:
 *  SM_EQ, SM_LT, SM_LE, SM_GT, SM_GE.
 *
 * Returns:
 *  Error code *   
 *    eBADCOMPOP_BTM
 *    eBADBTREEPAGE_BTM
 *    some errors caused by function calls
 */
Four edubtm_Fetch(
    PageID              *root,          /* IN The current root of the subtree */
    KeyDesc             *kdesc,         /* IN Btree key descriptor */
    KeyValue            *startKval,     /* IN key value of start condition */
    Four                startCompOp,    /* IN comparison operator of start condition */
    KeyValue            *stopKval,      /* IN key value of stop condition */
    Four                stopCompOp,     /* IN comparison operator of stop condition */
    BtreeCursor         *cursor)        /* OUT Btree Cursor */
{
    Four                e;              /* error number */
    Four                cmp;            /* result of comparison */
    Two                 idx;            /* index */
    PageID              child;          /* child page when the root is an internla page */
    Two                 alignedKlen;    /* aligned size of the key length */
    BtreePage           *apage;         /* a Page Pointer to the given root */
    BtreeOverflow       *opage;         /* a page pointer if it necessary to access an overflow page */
    Boolean             found;          /* search result */
    PageID              leafPid;       /* leaf page pointed by the cursor */
    Two                 slotNo;         /* slot pointed by the slot */
    PageID              ovPid;          /* PageID of the overflow page */
    PageNo              ovPageNo;       /* PageNo of the overflow page */
    PageID              prevPid;        /* PageID of the previous page */
    PageID              nextPid;        /* PageID of the next page */
    ObjectID            *oidArray;      /* array of the ObjectIDs */
    Two                 iEntryOffset;   /* starting offset of an internal entry */
    btm_InternalEntry   *iEntry;        /* an internal entry */
    Two                 lEntryOffset;   /* starting offset of a leaf entry */
    btm_LeafEntry       *lEntry;        /* a leaf entry */


    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    if(e = BfM_GetTrain(root, &apage, PAGE_BUF)) {
        ERR(e);
    }

    if(apage->any.hdr.type & INTERNAL) {
        found = edubtm_BinarySearchInternal(apage, kdesc, startKval, &idx);

        if(idx == -1) {
            MAKE_PAGEID(child, root->volNo, apage->bi.hdr.p0);
        }
        else {
            iEntry = &(apage->bi.data[apage->bi.slot[-1 * idx]]);

            MAKE_PAGEID(child, root->volNo, iEntry->spid);
        }

        if(e = edubtm_Fetch(&child, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor)) {
            BfM_FreeTrain(root, PAGE_BUF);
            ERR(e);
        }

        if(e = BfM_FreeTrain(root, PAGE_BUF)) {
            ERR(e);
        }
    } 
    else {
        leafPid = *root;

        found = edubtm_BinarySearchLeaf(apage, kdesc, startKval, &idx);
        if(found) {
            if(startCompOp == SM_EQ || startCompOp == SM_LE || startCompOp == SM_GE) {
                idx = idx;
            }
            else if(startCompOp == SM_LT || startCompOp == SM_GT) {
                if((idx == 0 && startCompOp == SM_LT) || (idx == apage->bl.hdr.nSlots - 1 && startCompOp == SM_GT)) {
                    if(startCompOp == SM_LT) {
                        MAKE_PAGEID(leafPid, root->volNo, apage->bl.hdr.prevPage); 
                    }
                    else {
                        MAKE_PAGEID(leafPid, root->volNo, apage->bl.hdr.nextPage);
                    }

                    if(e = BfM_FreeTrain(root, PAGE_BUF)) {
                        ERR(e);
                    }

                    if(leafPid.pageNo == NIL){
                        cursor->flag = CURSOR_EOS;
                     
                        return eNOERROR;
                    }

                    if(e = BfM_GetTrain(&leafPid, &apage, PAGE_BUF)) {
                        ERR(e);
                    }

                    if(startCompOp == SM_LT) {
                        idx = apage->bl.hdr.nSlots - 1;
                    }
                    else {
                        idx = 0;
                    }
                }
                else {
                    if(startCompOp == SM_LT) {
                        idx -= 1;
                    }
                    else {
                        idx += 1;
                    }
                }
            }
        }
        else {
            if(startCompOp == SM_EQ) {
                cursor->flag = CURSOR_EOS;

                if(e = BfM_FreeTrain(&leafPid, PAGE_BUF)) {
                    ERR(e);
                }

                return eNOERROR;
            }
            else if(startCompOp == SM_LT || startCompOp == SM_LE) {
                if(idx == -1) {
                    cursor->flag = CURSOR_EOS;
                    
                    if(e = BfM_FreeTrain(root, PAGE_BUF)) {
                        ERR(e);
                    }

                    return eNOERROR;
                }
                else{
                    idx = idx;
                }
            }
            else if(startCompOp == SM_GT || startCompOp == SM_GE) {
                if(idx != -1) {
                    if(idx == apage->bl.hdr.nSlots - 1) {
                        MAKE_PAGEID(leafPid, root->volNo, apage->bl.hdr.nextPage);

                        if(e = BfM_FreeTrain(root, PAGE_BUF)) {
                            ERR(e);
                        }

                        if(leafPid.pageNo == NIL) {
                            cursor->flag = CURSOR_EOS;
                         
                            return eNOERROR;
                        }

                        if(e = BfM_GetTrain(&leafPid, &apage, PAGE_BUF)) {
                            ERR(e);
                        }

                        idx = 0;
                    }
                    else {
                        idx += 1;
                    }
                }
                else {
                    idx = 0;
                }
            }  
        }

        lEntry = &(apage->bl.data[apage->bl.slot[-1 * idx]]);
        alignedKlen = ALIGNED_LENGTH(lEntry->klen);

        cursor->flag = CURSOR_ON;
        cursor->slotNo = idx;
        cursor->leaf = leafPid;
        cursor->key.len = lEntry->klen;
        memcpy(cursor->key.val, lEntry->kval, lEntry->klen);
        memcpy(&(cursor->oid), &(lEntry->kval) + alignedKlen, sizeof(ObjectID));
        
        if(stopCompOp == SM_GE || stopCompOp == SM_GT || stopCompOp == SM_LE || stopCompOp == SM_LT) {
            cmp = edubtm_KeyCompare(kdesc, &cursor->key, stopKval);
        
            if(cmp == EQUAL) {
                if(stopCompOp == SM_GT || stopCompOp == SM_LT) {
                    cursor->flag = CURSOR_EOS;
                }
            }
            else if(cmp == GREAT) {
                if(stopCompOp == SM_LE || stopCompOp == SM_LT) {
                    cursor->flag = CURSOR_EOS;
                }
            }
            else if(cmp == LESS) {
                if(stopCompOp == SM_GE || stopCompOp == SM_GT) {
                    cursor->flag = CURSOR_EOS;
                }
            }
        }
        
        if(e = BfM_FreeTrain(&leafPid, PAGE_BUF)) {
            ERR(e);
        }
    }

    return eNOERROR;
    
} /* edubtm_Fetch() */
