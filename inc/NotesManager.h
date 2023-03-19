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
#ifndef NOTESMANAGER_H_
#define NOTESMANAGER_H_

#include "Note.h"

using namespace Osp::Base::Collection;

enum SortType {
	SORT_BY_DATE,
	SORT_BY_TITLE,
	SORT_BY_TYPE
};

enum FilterType {
	FILTER_BY_TITLE,
	FILTER_BY_TEXT
};

class NotesManager {
public:
	NotesManager(void) {
		__dataPath = L"";
		__lastEntryId = 0;
	}
	virtual result Construct(const String &path);

	String GetPath(void) const { return __dataPath; }

	virtual result AddNote(Note *val);

	virtual result SerializeNotes(const ICollectionT<Note *> &pNotes);

	virtual result UpdateNote(Note *val) const;

	virtual result RemoveAll(void) const;

	virtual result RemoveNote(Note *val) const;
	virtual result RemoveNote(int entry_id) const;

	virtual LinkedListT<Note *> *GetNotesN(SortType sorting = SORT_BY_DATE, SortOrder order = SORT_ORDER_DESCENDING, NoteType type_filter = NOTE_TYPE_ALL, FilterType filter_mode = FILTER_BY_TITLE, const String &filter = L"") const;

private:
	result Load(void);

	String __dataPath;
	int __lastEntryId;
};

#endif
