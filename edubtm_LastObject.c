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
 * Module: edubtm_LastObject.c
 *
 * Description : 
 *  Find the last ObjectID of the given Btree.
 *
 * Exports:
 *  Four edubtm_LastObject(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*) 
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_LastObject()
 *================================*/
/*
 * Function:  Four edubtm_LastObject(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*) 
 *
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the last ObjectID of the given Btree. The 'cursor' will indicate
 *  the last ObjectID in the Btree, and it will be used as successive access
 *  by using the Btree.
 *
 * Returns:
 *  error code
 *    eBADPAGE_BTM
 *    some errors caused by function calls
 *
 * Side effects:
 *  cursor : the last ObjectID and its position in the Btree
 */
Four edubtm_LastObject(
    PageID   		*root,		/* IN the root of Btree */
    KeyDesc  		*kdesc,		/* IN key descriptor */
    KeyValue 		*stopKval,	/* IN key value of stop condition */
    Four     		stopCompOp,	/* IN comparison operator of stop condition */
    BtreeCursor 	*cursor)	/* OUT the last BtreeCursor to be returned */
{
	int			i;
    Four 		e;		/* error number */
    Four 		cmp;		/* result of comparison */
    BtreePage 		*apage;		/* pointer to the buffer holding current page */
    BtreeOverflow 	*opage;		/* pointer to the buffer holding overflow page */
    PageID 		curPid;		/* PageID of the current page */
    PageID 		child;		/* PageID of the child page */
    PageID 		ovPid;		/* PageID of the current overflow page */
    PageID 		nextOvPid;	/* PageID of the next overflow page */
    Two 		lEntryOffset;	/* starting offset of a leaf entry */
    Two 		iEntryOffset;	/* starting offset of an internal entry */
    btm_LeafEntry 	*lEntry;	/* a leaf entry */
    btm_InternalEntry 	*iEntry;	/* an internal entry */
    Four 		alignedKlen;	/* aligned length of the key length */
        

    if (root == NULL) ERR(eBADPAGE_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    MAKE_PAGEID(curPid, root->volNo, root->pageNo);
    
    if(e = BfM_GetTrain(root, &apage, PAGE_BUF)) {
        ERR(e);
    }
    while(apage->any.hdr.type & INTERNAL) {
        iEntry = &(apage->bi.data[apage->bi.slot[-(apage->bi.hdr.nSlots - 1)]]);
        
        MAKE_PAGEID(child, curPid.volNo, iEntry->spid);
        
        if(e = BfM_FreeTrain(&curPid, PAGE_BUF)) {
            ERR(e);
        }

        curPid = child;

        if(e = BfM_GetTrain(&curPid, &apage, PAGE_BUF)) {
            ERR(e);
        }
    }

    lEntry = &(apage->bl.data[apage->bl.slot[-(apage->bl.hdr.nSlots - 1)]]);
    alignedKlen = ALIGNED_LENGTH(lEntry->klen);

    cursor->slotNo = apage->bl.hdr.nSlots - 1;
    cursor->leaf = curPid;
    cursor->key.len = lEntry->klen;
    memcpy(cursor->key.val, lEntry->kval, lEntry->klen);
    memcpy(&(cursor->oid), &(lEntry->kval[alignedKlen]), sizeof(ObjectID));

    cmp = edubtm_KeyCompare(kdesc, &(cursor->key), stopKval);
    if(cmp == EQUAL) {
        cursor->flag = CURSOR_ON;

        if(stopCompOp == SM_GT) {
            cursor->flag = CURSOR_EOS;
        }

    }
    else if(cmp == GREAT) {
        cursor->flag = CURSOR_ON;
    }
    else if(cmp == LESS) {
        cursor->flag = CURSOR_EOS;
    }

    if(e = BfM_FreeTrain(&curPid, PAGE_BUF)) {
        ERR(e);
    }

    return eNOERROR;
    
} /* edubtm_LastObject() */