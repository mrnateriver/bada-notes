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
#ifndef CACHINGNOTESMANAGER_H_
#define CACHINGNOTESMANAGER_H_

#include "NotesManager.h"

using namespace Osp::Base::Runtime;

class CachingNotesManager;

class SerializerThread: public ITimerEventListener, public Thread {
public:
	SerializerThread(void);

	result Construct(CachingNotesManager *pS);

	static const long REQUEST_SERIALIZATION = 150;
private:
	bool OnStart(void);
	void OnStop(void);

	void OnTimerExpired(Timer& timer);
	virtual void OnUserEventReceivedN(RequestId requestId, IList *pArgs);

	Timer* __pTimer;
	CachingNotesManager *__pSerializer;
};

#define DEF_COMPARER(x) class x: public IComparerT<Note *> {												\
						public:																				\
							x(bool ascending): __asc(ascending) { }											\
						private:																			\
							virtual result Compare(Note* const &obj1, Note* const &obj2, int &cmp) const;	\
							bool __asc;																		\
						}

DEF_COMPARER(NoteTitleComparer);
DEF_COMPARER(NoteDateComparer);
DEF_COMPARER(NoteTypeComparer);

class CachingNotesManager: public NotesManager {
public:
	CachingNotesManager();
	virtual ~CachingNotesManager();

	virtual result Construct(const String &path);

	virtual result AddNote(Note *val);

	virtual result UpdateNote(Note *val);

	virtual result RemoveNote(Note *val);
	virtual result RemoveNote(int entry_id);

	virtual LinkedListT<Note *> *GetNotesN(SortType sorting = SORT_BY_DATE, SortOrder order = SORT_ORDER_DESCENDING, NoteType type_filter = NOTE_TYPE_ALL, FilterType filter_mode = FILTER_BY_TITLE, const String &filter = L"");

	Note *GetNote(int index) const;

private:
	LinkedListT<Note *> *__pNotes;
	SerializerThread *__pSerializer;
	bool __smtChanged;
	bool __12APIAvailable;

	friend class SerializerThread;
};

#endif
