/*
 * Copyright (c) 2016 Evgenii Dobrovidov
 * This file is part of "Notes".
 *
 * "Notes" is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * "Notes" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with "Notes".  If not, see <http://www.gnu.org/licenses/>.
 */
#include <FSystem.h>

#include "CachingNotesManager.h"

using namespace Osp::System;

SerializerThread::SerializerThread(void) {
	__pTimer = null;
}

result SerializerThread::Construct(CachingNotesManager *pS) {
	__pSerializer = pS;
	return Thread::Construct(THREAD_TYPE_EVENT_DRIVEN);
}

bool SerializerThread::OnStart(void) {
   __pTimer = new Timer;
   result res = __pTimer->Construct(*this);
   if (IsFailed(res)) {
	   AppLogException("Failed to construct timer object in serializer thread, error: [%s]", GetErrorMessage(res));

	   SetLastResult(res);
	   return false;
   }

   res = __pTimer->Start(60*1000);
   if (IsFailed(res)) {
	   AppLogException("Failed to start timer in serializer thread, error: [%s]", GetErrorMessage(res));

	   SetLastResult(res);
	   return false;
   } else return true;
}

void SerializerThread::OnStop(void) {
   result res = __pTimer->Cancel();
   if (IsFailed(res)) {
	   AppLogException("Failed to cease timer in serializer thread, error: [%s]", GetErrorMessage(res));
	   SetLastResult(res);
   }
   delete __pTimer;
}

void SerializerThread::OnTimerExpired(Timer& timer) {
	AppLogDebug("OnTimerExpired event!");
	if (__pSerializer->__smtChanged) {
		__pSerializer->SerializeNotes(*__pSerializer->__pNotes);
	}

	result res = __pTimer->Start(60*1000);
	if (IsFailed(res)) {
		AppLogException("Failed to restart timer in serializer thread, error: [%s]", GetErrorMessage(res));
		SetLastResult(res);
	}
}

void SerializerThread::OnUserEventReceivedN(RequestId requestId, IList *pArgs) {
	AppLogDebug("Received serialization event!");
	if (requestId == REQUEST_SERIALIZATION) {
		__pTimer->Cancel();
		OnTimerExpired(*__pTimer);
	}
}

result NoteTitleComparer::Compare(Note* const &obj1, Note* const &obj2, int &cmp) const {
	if (obj1->GetMarked() && !obj2->GetMarked()) {
		cmp = -1;
	} else if (!obj1->GetMarked() && obj2->GetMarked()) {
		cmp = 1;
	} else {
		int ret = obj1->GetTitle().CompareTo(obj2->GetTitle());
		cmp = __asc ? -ret : ret;
	}
	return E_SUCCESS;
}

result NoteDateComparer::Compare(Note* const &obj1, Note* const &obj2, int &cmp) const {
	if (obj1->GetMarked() && !obj2->GetMarked()) {
		cmp = -1;
	} else if (!obj1->GetMarked() && obj2->GetMarked()) {
		cmp = 1;
	} else {
		int ret = obj2->GetDate() - obj1->GetDate();
		cmp = __asc ? -ret : ret;
	}
	return E_SUCCESS;
}

result NoteTypeComparer::Compare(Note* const &obj1, Note* const &obj2, int &cmp) const {
	if (obj1->GetMarked() && !obj2->GetMarked()) {
		cmp = -1;
	} else if (!obj1->GetMarked() && obj2->GetMarked()) {
		cmp = 1;
	} else {
		int ret = (int) obj2->GetType() - (int) obj1->GetType();
		cmp = __asc ? -ret : ret;
	}
	return E_SUCCESS;
}

CachingNotesManager::CachingNotesManager() {
	__pNotes = null;
	__pSerializer = null;
	__12APIAvailable = null;
	__smtChanged = false;
}

CachingNotesManager::~CachingNotesManager() {
	if (__pNotes) delete __pNotes;
	if (__pSerializer) {
		__pSerializer->Stop();
		delete __pSerializer;
	}
}

result CachingNotesManager::Construct(const String &path) {
    String platformVersion;
    result res = SystemInfo::GetValue(L"APIVersion", platformVersion);
    if (!IsFailed(res)) {
    	int dotIndex = -1;
    	res = platformVersion.IndexOf(L".", 0, dotIndex);
    	if (IsFailed(res)) {
    		AppLogException("Failed to parse API version string, string operations will be case sensitive. Error: [%s]", GetErrorMessage(res));
    		return res;
    	}

    	String majorStr(L"1");
    	res = platformVersion.SubString(0, dotIndex, majorStr);
    	if (IsFailed(res)) {
    		AppLogException("Failed to parse API version string, string operations will be case sensitive. Error: [%s]", GetErrorMessage(res));
    		return res;
    	}

    	int majorVersion = 1;
    	res = Integer::Parse(majorStr, majorVersion);
    	if (IsFailed(res)) {
    		AppLogException("Failed to parse API version string, string operations will be case sensitive. Error: [%s]", GetErrorMessage(res));
    		return res;
    	}

    	String minorStr(L"0");
    	res = platformVersion.SubString(dotIndex + 1, minorStr);
    	if (IsFailed(res)) {
    		AppLogException("Failed to parse API version string, string operations will be case sensitive. Error: [%s]", GetErrorMessage(res));
    		return res;
    	}

    	int minorVersion = 1;
    	res = Integer::Parse(minorStr, minorVersion);
    	if (IsFailed(res)) {
    		AppLogException("Failed to parse API version string, string operations will be case sensitive. Error: [%s]", GetErrorMessage(res));
    		return res;
    	}

    	if (majorVersion >= 1 && minorVersion >= 2) {
    		__12APIAvailable = true;
    	}
    } else {
		AppLogException("Failed to acquire API version string, string operations will be case sensitive. Error: [%s]", GetErrorMessage(res));
		return res;
    }

    __pSerializer = new SerializerThread;
    __pSerializer->Construct(this);

	res = NotesManager::Construct(path);
	if (IsFailed(res)) {
		AppLogException("Failed to construct underlying notes manager for serialization, error: [%s]", GetErrorMessage(res));
		return res;
	}

	__pNotes = NotesManager::GetNotesN();
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to precache notes, error: [%s]", GetErrorMessage(res));
		return res;
	}

	__pSerializer->Start();

	return E_SUCCESS;
}

result CachingNotesManager::AddNote(Note *val) {
	if (__pNotes) {
		__smtChanged = true;
		result res = __pNotes->Add(val);

		if (!IsFailed(res)) __pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		return res;
	} else {
		AppLogException("Attempt to add note to the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingNotesManager::UpdateNote(Note *val) {
	if (__pNotes) {
		__smtChanged = true;
		__pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		return E_SUCCESS;
	} else {
		AppLogException("Attempt to update note in the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingNotesManager::RemoveNote(Note *val) {
	if (__pNotes) {
		__smtChanged = true;
		result res = __pNotes->Remove(val);

		if (!IsFailed(res)) __pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		return res;
	} else {
		AppLogException("Attempt to remove note from the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingNotesManager::RemoveNote(int entry_id) {
	if (__pNotes) {
		IEnumeratorT<Note *> *pEnum = __pNotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			return E_INVALID_CONDITION;
		}

		Note *pToRemove = null;
		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Note *pNote; pEnum->GetCurrent(pNote);
				if (pNote->GetEntryId() == entry_id) {
					pToRemove = pNote;
					break;
				}
			}
		}

		delete pEnum;

		if (pToRemove) {
			__smtChanged = true;
			res = __pNotes->Remove(pToRemove);

			if (!IsFailed(res)) __pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
			return res;
		} else {
			return E_OBJ_NOT_FOUND;
		}
	} else {
		AppLogException("Attempt to remove note from the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

LinkedListT<Note *> *CachingNotesManager::GetNotesN(SortType sorting, SortOrder order, NoteType type_filter, FilterType filter_mode, const String &filter) {
	result res = E_SUCCESS;
	if (__pNotes) {
		if (sorting == SORT_BY_DATE) {
			__pNotes->Sort(NoteDateComparer(order == SORT_ORDER_ASCENDING ? true : false));
		} else if (sorting == SORT_BY_TYPE) {
			__pNotes->Sort(NoteTypeComparer(order == SORT_ORDER_ASCENDING ? true : false));
		} else {
			__pNotes->Sort(NoteTitleComparer(order == SORT_ORDER_ASCENDING ? true : false));
		}

		LinkedListT<Note *> *__pRet = new LinkedListT<Note *> ;

		IEnumeratorT<Note *> *pEnum = __pNotes->GetEnumeratorN();
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			delete __pRet;
			return null;
		}

		String search_str;
		if (__12APIAvailable && !filter.IsEmpty()) {
			filter.ToLowerCase(search_str);
		} else {
			search_str = filter;
		}

		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Note *pNote; pEnum->GetCurrent(pNote);
				if (!filter.IsEmpty()) {

					int sIndex = -1;
					String tmp;
					if (filter_mode == FILTER_BY_TEXT) {
						tmp = pNote->GetText();
					} else {
						tmp = pNote->GetTitle();
					}

					if (__12APIAvailable) {
						tmp.ToLowerCase();
					}
					res = tmp.IndexOf(search_str, 0, sIndex);

					if (res == E_OBJ_NOT_FOUND) {
						continue;
					}
				}
				if (type_filter != NOTE_TYPE_ALL && type_filter != pNote->GetType()) {
					continue;
				}
				__pRet->Add(pNote);
			}
		}

		delete pEnum;

		return __pRet;
	} else {
		AppLogException("Attempt to get notes list when it wasn't pre-cached yet");
		SetLastResult(E_INVALID_STATE);
		return null;
	}
}

Note *CachingNotesManager::GetNote(int index) const {
	if (__pNotes) {
		Note *pRet;
		result res = __pNotes->GetAt(index, pRet);
		SetLastResult(res);
		if (IsFailed(res)) {
			return null;
		} else {
			return pRet;
		}
	} else {
		AppLogException("Attempt to get note from the list which wasn't cached yet");
		SetLastResult(E_INVALID_STATE);
		return null;
	}
}
