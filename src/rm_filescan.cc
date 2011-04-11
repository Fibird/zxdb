//
// File:        rm_record.cc
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm.h"

using namespace std;

RM_FileScan::RM_FileScan(): bOpen(false)
{
	pred = NULL;
	prmh = NULL;
	current = RID(1,-1);
}

RM_FileScan::~RM_FileScan()
{
}

RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,
												 AttrType   attrType,	     
												 int        attrLength,
												 int        attrOffset,
												 CompOp     compOp,
												 void       *value,
												 ClientHint pinHint) 
{
	bOpen = true;
	pred = new Predicate(                   attrType,	     
																					attrLength,
																					attrOffset,
																					compOp,
																					value,
																					pinHint) ;
	prmh = const_cast<RM_FileHandle*>(&fileHandle);
	return 0;
}

RC RM_FileScan::GetNextRec     (RM_Record &rec)
{
	assert(bOpen);
	if(!bOpen)
		return RM_FNOTOPEN;

	PF_PageHandle ph;
	RM_PageHdr pHdr(prmh->GetNumSlots());
	RC rc;
	
	for( int j = current.Page(); j < prmh->GetNumPages(); j++) {
		if(rc = prmh->pfHandle->GetThisPage(j, ph))
			return rc;
		if(rc = prmh->GetPageHeader(ph, pHdr))
			return rc;
		bitmap b(pHdr.freeSlotMap, prmh->GetNumSlots());
		int i = -1;
		if(current.Page() == j)
				i = current.Slot()+1;
		else
			i = 0;
		for (; i < prmh->GetNumSlots(); i++) 
		{
			if (!b.test(i)) { 
				// not free - means this is useful data to us
				current = RID(j, i);
				prmh->GetRec(current, rec);
				std::cerr << "GetNextRec ret RID " << current << std::endl;
				char * pData = NULL;
				rec.GetData(pData);
				if(pred->eval(pData, pred->initOp())) {
					std::cerr << "GetNextRec pred match for RID " << current << std::endl;
					return 0;
				} else {
					// get next rec
				}
			}
		}
	}
	return RM_EOF;
	
	// for(PageNum p = current.Page(); p <= prmh->GetNumPages(); p++) {
	// 		for(SlotNum s = current.Slot(); s <= prmh->GetNumSlots(); s++) {
	// 			prmh.GetRec(RID(p,s), rec);
	// 		}
	// }
}

RC RM_FileScan::CloseScan()
{
	if(!bOpen)
		return RM_FNOTOPEN;
	bOpen = false;
	if (pred != NULL)
		delete pred;
	return 0;
}
