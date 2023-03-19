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
#ifndef NOTE_H_
#define NOTE_H_

#include <FBase.h>

using namespace Osp::Base;

enum NoteType {
	NOTE_TYPE_ALL = 0,
	NOTE_TYPE_TEXT,
	NOTE_TYPE_PHOTO,
	NOTE_TYPE_AUDIO,
	//reserved for future use:
	NOTE_TYPE_VIDEO,
	NOTE_TYPE_MAP
};

class Note {
public:
	Note(void);

	result Construct(NoteType type);

	int GetEntryId(void) const { return __entryId; }
	NoteType GetType(void) const { return __type; }

	bool GetSerialized(void) const {
		return __serialized;
	}
	void SetSerialized(bool val) {
		__serialized = val;
	}

	long long GetDate(void) const {
		return __modTime;
	}
	void SetDate(long long modTime) {
		__modTime = modTime;
	}

	bool GetMarked(void) const {
		return __marked;
	}
	void SetMarked(bool val) {
		__marked = val;
	}

	void SetText(const String &text) {
		__text = text;
	}
	String GetText(void) const {
		return __text;
	}

	void SetTitle(const String &title) {
		__title = title;
	}
	String GetTitle(void) const {
		return __title;
	}

	void SetResourcePath(const String &resPath) {
		__resPath = resPath;
	}
	String GetResourcePath(void) const {
		return __resPath;
	}

private:
	bool __serialized;

	int __entryId;
	NoteType __type;
	long long __modTime;
	bool __marked;

	String __text;
	String __title;
	String __resPath;

	void SetEntryID(int id) {
		__entryId = id;
	}

	friend class NotesManager;
};

#endif
