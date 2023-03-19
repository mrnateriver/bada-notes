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

#include "Note.h"

Note::Note(void) {
	__serialized = false;

	__entryId = -1;
	__type = NOTE_TYPE_TEXT;
	__modTime = 0;
	__marked = false;

	__text = L"";
	__title = L"";
	__resPath = L"";
}

result Note::Construct(NoteType type) {
	__type = type;
	return E_SUCCESS;
}
