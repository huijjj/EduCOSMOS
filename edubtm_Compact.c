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
 * Module: edubtm_Compact.c
 * 
 * Description:
 *  Two functions edubtm_CompactInternalPage() and edubtm_CompactLeafPage() are
 *  used to compact the internal page and the leaf page, respectively.
 *
 * Exports:
 *  void edubtm_CompactInternalPage(BtreeInternal*, Two)
 *  void edubtm_CompactLeafPage(BtreeLeaf*, Two)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_CompactInternalPage()
 *================================*/
/*
 * Function: edubtm_CompactInternalPage(BtreeInternal*, Two)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Reorganize the internal page to make sure the unused bytes in the page
 *  are located contiguously "in the middle", between the entries and the
 *  slot array. To compress out holes, entries must be moved toward the
 *  beginning of the page.
 *
 * Returns:
 *  None
 *
 * Side effects:
 *  The leaf page is reorganized to compact the space.
 */
void edubtm_CompactInternalPage(
    BtreeInternal       *apage,                 /* INOUT internal page to compact */
    Two                 slotNo)                 /* IN slot to go to the boundary of free space */
{
    BtreeInternal       tpage;                  /* temporay page used to save the given page */
    Two                 apageDataOffset;        /* where the next object is to be moved */
    Two                 len;                    /* length of the leaf entry */
    Two                 i;                      /* index variable */
    btm_InternalEntry   *entry;                 /* an entry in leaf page */

    memcpy(&tpage, apage, PAGESIZE);

	apageDataOffset = 0;
	for(i = 0; i < apage->hdr.nSlots; i++) {
		if(i != slotNo) {
			entry = &(tpage.data[tpage.slot[-1 * i]]);
			len = sizeof(ShortPageID) + 2 + ALIGNED_LENGTH(entry->klen);
			
			apage->slot[-1 * i] = apageDataOffset;
            memcpy(&(apage->data[apageDataOffset]), entry, len);
			
            apageDataOffset += len;
		}
	}

	if(slotNo != NIL) {
		entry = &(tpage.data[tpage.slot[-1*slotNo]]);
		len = sizeof(ShortPageID) + 2 + ALIGNED_LENGTH(entry->klen);

		apage->slot[-1 * slotNo] = apageDataOffset;
		memcpy(&(apage->data[apageDataOffset]), entry, len);
		
        apageDataOffset += len;
	}

	apage->hdr.unused = 0; 
	apage->hdr.free = apageDataOffset;
} /* edubtm_CompactInternalPage() */



/*@================================
 * edubtm_CompactLeafPage()
 *================================*/
/*
 * Function: void edubtm_CompactLeafPage(BtreeLeaf*, Two)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Reorganizes the leaf page to make sure the unused bytes in the page
 *  are located contiguously "in the middle", between the entries and the
 *  slot array. To compress out holes, entries must be moved toward the
 *  beginning of the page.
 *	
 * Return Values :
 *  None
 *
 * Side Effects :
 *  The leaf page is reorganized to comact the space.
 */
void edubtm_CompactLeafPage(
    BtreeLeaf 		*apage,			/* INOUT leaf page to compact */
    Two       		slotNo)			/* IN slot to go to the boundary of free space */
{	
    BtreeLeaf 		tpage;			/* temporay page used to save the given page */
    Two                 apageDataOffset;        /* where the next object is to be moved */
    Two                 len;                    /* length of the leaf entry */
    Two                 i;                      /* index variable */
    btm_LeafEntry 	*entry;			/* an entry in leaf page */
    Two 		alignedKlen;		/* aligned length of the key length */

    memcpy(&tpage, apage, PAGESIZE);

	apageDataOffset = 0;
	for(i = 0; i < apage->hdr.nSlots; i++) {
		if(i != slotNo) {
			entry = &(tpage.data[tpage.slot[-1 * i]]);
			len = 4 + ALIGNED_LENGTH(entry->klen) + sizeof(ObjectID);
			
			apage->slot[-1 * i] = apageDataOffset;
            memcpy(&(apage->data[apageDataOffset]), entry, len);
			
            apageDataOffset += len;
		}
	}

	if(slotNo != NIL) {
		entry = &(tpage.data[tpage.slot[-1*slotNo]]);
		len = 4 + ALIGNED_LENGTH(entry->klen) + sizeof(ObjectID);
		
        apage->slot[-1 * slotNo] = apageDataOffset;
		memcpy(&(apage->data[apageDataOffset]), entry, len);
		
        apageDataOffset += len;
	}

	apage->hdr.unused = 0;
	apage->hdr.free = apageDataOffset;
} /* edubtm_CompactLeafPage() */