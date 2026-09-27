#include "hexeditwnd.h"

HE_OPER::HE_OPER(EDIT_OPER optype, ULONGLONG offset, ULONGLONG oldlen, ULONGLONG newlen) {
	type		= optype;
	qwOffset	= offset;
	qwOldLen	= oldlen;
	qwNewLen	= newlen;
	qwOldAdd	= (ULONGLONG)-1;
	qwNewAdd	= (ULONGLONG)-1;
	if (oldlen >= 1) {
		oldData	= new BYTE[(SIZE_T)oldlen];
	} else {
		oldData = NULL;
	}

	if (newlen >= 1) {
		newData	= new BYTE[(SIZE_T)newlen];
	} else {
		newData = NULL;
	}
}

// No-heap-data constructor for paged large-file ops: byte spans live in
// the add store (qwOldAdd/qwNewAdd) so multi-GB delete/paste never needs
// a contiguous heap staging buffer on 32-bit.
HE_OPER::HE_OPER(EDIT_OPER optype, ULONGLONG offset, ULONGLONG oldlen, ULONGLONG newlen, BOOL) {
	type		= optype;
	qwOffset	= offset;
	qwOldLen	= oldlen;
	qwNewLen	= newlen;
	oldData		= NULL;
	newData		= NULL;
	qwOldAdd	= (ULONGLONG)-1;
	qwNewAdd	= (ULONGLONG)-1;
}

HE_OPER::~HE_OPER() {
	delete [] newData;
	delete [] oldData;
}

EditOperList::EditOperList() {
	next = NULL;
	prev = NULL;
	oper = NULL;
}

EditOperList::~EditOperList() {
	if (next != NULL) {
		delete next;
	}

	delete oper;
}

EditOperList* EditOperList::getFirst() {
	EditOperList *temp;

	temp = this;
	while (temp->prev != NULL) {
		temp = temp->prev;
	}

	return temp;
}

EditOperList* EditOperList::getLast() {
	EditOperList *temp;

	temp = this;
	while (temp->next != NULL) {
		temp = temp->next;
	}

	return temp;
}

EditOperList* EditOperList::getNext() {
	return next;
}

EditOperList* EditOperList::getPrev() {
	return prev;
}

HE_OPER* EditOperList::getOper() {
	return oper;
}

EditOperList* EditOperList::addOper(HE_OPER* op) {
	EditOperList *last;
	EditOperList *newop;

	newop = new EditOperList();
	newop->oper = op;

	last = getLast();
	last->next = newop;
	newop->prev = last;
	return newop;
}

void EditOperList::releaseAfter() {
	if (next != NULL) {
		delete next;
	}
	next = NULL;
}

BOOL EditOperList::before(EditOperList *list) {
	EditOperList *tmp;

	tmp = next;
	while (tmp != NULL) {
		if (tmp == list) {
			return TRUE;
		}
		tmp = tmp->next;
	}

	return FALSE;
}
