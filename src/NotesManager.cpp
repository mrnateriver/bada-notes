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
#include <FApp.h>
#include <FIo.h>

#include "NotesManager.h"

using namespace Osp::App;
using namespace Osp::Io;

#define DB_VERSION 2

result NotesManager::Construct(const String &path) {
	__dataPath = path;
	return Load();
}

result NotesManager::Load(void) {
	if (File::IsFileExist(__dataPath)) {
		Database *pDb = new Database;
		result res = pDb->Construct(__dataPath, false);
		if (IsFailed(res)) {
			AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		DbEnumerator *pEnum = pDb->QueryN(L"SELECT ver FROM db_info"); res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to query database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}
		if (pEnum) {
			while (!IsFailed(pEnum->MoveNext())) {
				int ver;
				res = pEnum->GetIntAt(0, ver);
				if (IsFailed(res)) {
					AppLogException("Failed to get version data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

					delete pEnum;
					delete pDb;
					return res;
				}

				if (DB_VERSION != ver) {
					delete pEnum;
					delete pDb;

					return E_INVALID_FORMAT;
				} else break;
			}
		} else {
			delete pDb;
			return E_INVALID_FORMAT;
		}
		delete pEnum;

		pEnum = pDb->QueryN(L"SELECT MAX(entry_id) FROM entries"); res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to query database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		if (pEnum) {
			while (!IsFailed(pEnum->MoveNext())) {
				int maxID;
				res = pEnum->GetIntAt(0, maxID);
				if (IsFailed(res)) {
					AppLogException("Failed to get last entry ID for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

					delete pEnum;
					delete pDb;
					return res;
				}
				__lastEntryId = ++maxID;
				break;
			}
		} else {
			delete pDb;
			return E_INVALID_FORMAT;
		}
		delete pEnum;
		delete pDb;
	} else {
		Database *pDb = new Database;
		result res = pDb->Construct(__dataPath, true);
		if (IsFailed(res)) {
			AppLogException("Failed to initialize database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		result mres[4];
		mres[0] = pDb->ExecuteSql(L"CREATE TABLE db_info (ver INTEGER)", true);
		String db_ver = L""; db_ver.Append(DB_VERSION);
		mres[1] = pDb->ExecuteSql(L"INSERT INTO db_info (ver) VALUES ('" + db_ver + L"')", true);

		mres[2] = pDb->ExecuteSql(L"CREATE TABLE entries (entry_id INTEGER, type INTEGER, timestamp INTEGER, marked INTEGER, title TEXT, text TEXT)", true);
		mres[3] = pDb->ExecuteSql(L"CREATE TABLE resource_entries (entry_id INTEGER, res_path TEXT)", true);

		delete pDb;

		for(int i = 0; i < 4; i++) {
			if (IsFailed(mres[i])) {
				AppLogException("Failed to initialize database structure at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(mres[i]));

				return mres[i];
			}
		}
	}
	return E_SUCCESS;
}

result NotesManager::AddNote(Note *val) {
	if (val->GetEntryId() >= 0) {
		return UpdateNote(val);
	}

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, true);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result trans_res[2];
	trans_res[0] = pDb->BeginTransaction();

	DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO entries (entry_id, type, timestamp, marked, title, text) VALUES (?, ?, ?, ?, ?, ?)");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	int cur_id = __lastEntryId++;

	result bind_res[6];
	bind_res[0] = pEntries->BindInt(0, cur_id);
	bind_res[1] = pEntries->BindInt(1, val->GetType());
	bind_res[2] = pEntries->BindInt64(2, val->GetDate());
	bind_res[3] = pEntries->BindInt(3, (int)val->GetMarked());
	bind_res[4] = pEntries->BindString(4, val->GetTitle());
	bind_res[5] = pEntries->BindString(5, val->GetText());

	for(int i = 0; i < 6; i++) {
		if (IsFailed(bind_res[i])) {
			AppLogException("Failed to bind transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

			delete pEntries;
			delete pDb;
			return bind_res[i];
		}
	}

	pDb->ExecuteStatementN(*pEntries); res = GetLastResult();
	delete pEntries;

	if (IsFailed(res)) {
		AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	if (val->GetType() == NOTE_TYPE_PHOTO || val->GetType() == NOTE_TYPE_AUDIO) {
		DbStatement *pText = pDb->CreateStatementN(L"INSERT INTO resource_entries (entry_id, res_path) VALUES (?, ?)");
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to initialize resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		bind_res[0] = pText->BindInt(0, cur_id);
		bind_res[1] = pText->BindString(1, val->GetResourcePath());

		for(int i = 0; i < 2; i++) {
			if (IsFailed(bind_res[i])) {
				AppLogException("Failed to bind resource transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

				delete pText;
				delete pDb;
				return bind_res[i];
			}
		}

		pDb->ExecuteStatementN(*pText); res = GetLastResult();
		delete pText;

		if (IsFailed(res)) {
			AppLogException("Failed to execute resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pEntries;
			delete pDb;
			return res;
		}
	}

	trans_res[1] = pDb->CommitTransaction();
	delete pDb;

	for(int i = 0; i < 2; i++) {
		if (IsFailed(trans_res[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(trans_res[i]));

			return trans_res[i];
		}
	}

	val->SetEntryID(cur_id);
	val->SetSerialized(true);

	return E_SUCCESS;
}

result NotesManager::SerializeNotes(const ICollectionT<Note *> &pNotes) {
	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, true);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result trans_res[2];
	trans_res[0] = pDb->BeginTransaction();

	DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO entries (entry_id, type, timestamp, marked, title, text) VALUES (?, ?, ?, ?, ?, ?)");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	DbStatement *pText = pDb->CreateStatementN(L"INSERT INTO resource_entries (entry_id, res_path) VALUES (?, ?)");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		delete pEntries;
		return res;
	}

	DbStatement *pUpdate = pDb->CreateStatementN(L"UPDATE entries SET timestamp = ?, marked = ?, title = ?, text = ? WHERE entry_id = ?");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		delete pEntries;
		delete pText;
		return res;
	}

	DbStatement *pUpdateRes = pDb->CreateStatementN(L"UPDATE resource_entries SET res_path = ? WHERE entry_id = ?");
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		delete pEntries;
		delete pText;
		delete pUpdate;
		return res;
	}

	IEnumeratorT<Note *> *pEnum = pNotes.GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Note *pNote; pEnum->GetCurrent(pNote);
		if (pNote->GetSerialized()) {
			continue;
		}
		if (pNote->GetEntryId() >= 0) {
			result bind_res[5];
			bind_res[0] = pUpdate->BindInt64(0, pNote->GetDate());
			bind_res[1] = pUpdate->BindInt(1, (int)pNote->GetMarked());
			bind_res[2] = pUpdate->BindString(2, pNote->GetTitle());
			bind_res[3] = pUpdate->BindString(3, pNote->GetText());
			bind_res[4] = pUpdate->BindInt(4, pNote->GetEntryId());

			for(int i = 0; i < 5; i++) {
				if (IsFailed(bind_res[i])) {
					AppLogException("Failed to bind transaction data with index [%d] for database at [%S], error: [%s]", i, __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

					delete pText;
					delete pEntries;
					delete pDb;
					delete pEnum;
					delete pUpdate;
					delete pUpdateRes;
					return bind_res[i];
				}
			}

			pDb->ExecuteStatementN(*pUpdate); res = GetLastResult();

			if (IsFailed(res)) {
				AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

				delete pText;
				delete pEntries;
				delete pDb;
				delete pEnum;
				delete pUpdate;
				delete pUpdateRes;
				return res;
			}

			if (pNote->GetType() == NOTE_TYPE_PHOTO || pNote->GetType() == NOTE_TYPE_AUDIO) {
				result bind_res[2];
				bind_res[0] = pUpdateRes->BindString(0, pNote->GetResourcePath());
				bind_res[1] = pUpdateRes->BindInt(1, pNote->GetEntryId());

				for(int i = 0; i < 2; i++) {
					if (IsFailed(bind_res[i])) {
						AppLogException("Failed to bind resource transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

						delete pText;
						delete pEntries;
						delete pDb;
						delete pEnum;
						delete pUpdate;
						delete pUpdateRes;
						return bind_res[i];
					}
				}

				pDb->ExecuteStatementN(*pUpdateRes); res = GetLastResult();
				if (IsFailed(res)) {
					AppLogException("Failed to execute resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

					delete pText;
					delete pEntries;
					delete pDb;
					delete pEnum;
					delete pUpdate;
					delete pUpdateRes;
					return res;
				}
			}
		} else {
			int cur_id = __lastEntryId++;

			result bind_res[6];
			bind_res[0] = pEntries->BindInt(0, cur_id);
			bind_res[1] = pEntries->BindInt(1, pNote->GetType());
			bind_res[2] = pEntries->BindInt64(2, pNote->GetDate());
			bind_res[3] = pEntries->BindInt(3, (int)pNote->GetMarked());
			bind_res[4] = pEntries->BindString(4, pNote->GetTitle());
			bind_res[5] = pEntries->BindString(5, pNote->GetText());

			for(int i = 0; i < 6; i++) {
				if (IsFailed(bind_res[i])) {
					AppLogException("Failed to bind transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

					delete pText;
					delete pEntries;
					delete pDb;
					delete pEnum;
					delete pUpdate;
					delete pUpdateRes;
					return bind_res[i];
				}
			}

			pDb->ExecuteStatementN(*pEntries); res = GetLastResult();
			if (IsFailed(res)) {
				AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

				delete pText;
				delete pEntries;
				delete pDb;
				delete pEnum;
				delete pUpdate;
				delete pUpdateRes;
				return res;
			}

			if (pNote->GetType() == NOTE_TYPE_PHOTO || pNote->GetType() == NOTE_TYPE_AUDIO) {
				bind_res[0] = pText->BindInt(0, cur_id);
				bind_res[1] = pText->BindString(1, pNote->GetResourcePath());

				for(int i = 0; i < 2; i++) {
					if (IsFailed(bind_res[i])) {
						AppLogException("Failed to bind resource transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

						delete pText;
						delete pEntries;
						delete pDb;
						delete pEnum;
						delete pUpdate;
						delete pUpdateRes;
						return bind_res[i];
					}
				}

				pDb->ExecuteStatementN(*pText); res = GetLastResult();
				if (IsFailed(res)) {
					AppLogException("Failed to execute resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

					delete pText;
					delete pEntries;
					delete pDb;
					delete pEnum;
					delete pUpdate;
					delete pUpdateRes;
					return res;
				}
			}

			pNote->SetEntryID(cur_id);
		}
		pNote->SetSerialized(true);
	}
	delete pEnum;
	delete pEntries;
	delete pText;
	delete pUpdate;
	delete pUpdateRes;

	trans_res[1] = pDb->CommitTransaction();
	delete pDb;

	for(int i = 0; i < 2; i++) {
		if (IsFailed(trans_res[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(trans_res[i]));

			return trans_res[i];
		}
	}
	return E_SUCCESS;
}

result NotesManager::UpdateNote(Note *val) const {
	if (val->GetEntryId() < 0) {
		AppLogException("Attempt to update note that is not yet saved to database");
		return E_INVALID_ARG;
	} else if (val->GetSerialized()) {
		AppLogException("Attempt to update note that is already serialized");
		return E_INVALID_ARG;
	}

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, true);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result trans_res[2];
	trans_res[0] = pDb->BeginTransaction();

	DbStatement *pEntries = pDb->CreateStatementN(L"UPDATE entries SET timestamp = ?, marked = ?, title = ?, text = ? WHERE entry_id = ?");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result bind_res[5];
	bind_res[0] = pEntries->BindInt64(0, val->GetDate());
	bind_res[1] = pEntries->BindInt(1, (int)val->GetMarked());
	bind_res[2] = pEntries->BindString(2, val->GetTitle());
	bind_res[3] = pEntries->BindString(3, val->GetText());
	bind_res[4] = pEntries->BindInt64(4, val->GetEntryId());

	for(int i = 0; i < 5; i++) {
		if (IsFailed(bind_res[i])) {
			AppLogException("Failed to bind transaction data with index [%d] for database at [%S], error: [%s]", i, __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

			delete pEntries;
			delete pDb;
			return bind_res[i];
		}
	}

	pDb->ExecuteStatementN(*pEntries); res = GetLastResult();
	delete pEntries;

	if (IsFailed(res)) {
		AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	if (val->GetType() == NOTE_TYPE_PHOTO || val->GetType() == NOTE_TYPE_AUDIO) {
		DbStatement *pText = pDb->CreateStatementN(L"UPDATE resource_entries SET res_path = ? WHERE entry_id = ?");
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to initialize resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		bind_res[0] = pText->BindString(0, val->GetResourcePath());
		bind_res[1] = pText->BindInt(1, val->GetEntryId());

		for(int i = 0; i < 2; i++) {
			if (IsFailed(bind_res[i])) {
				AppLogException("Failed to bind resource transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

				delete pText;
				delete pDb;
				return bind_res[i];
			}
		}

		pDb->ExecuteStatementN(*pText); res = GetLastResult();
		delete pText;

		if (IsFailed(res)) {
			AppLogException("Failed to execute resource transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pEntries;
			delete pDb;
			return res;
		}
	}

	trans_res[1] = pDb->CommitTransaction();
	delete pDb;

	for(int i = 0; i < 2; i++) {
		if (IsFailed(trans_res[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(trans_res[i]));

			return trans_res[i];
		}
	}

	val->SetSerialized(true);

	return E_SUCCESS;
}

result NotesManager::RemoveAll(void) const {
	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, true);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result mres[4];
	mres[0] = pDb->BeginTransaction();

	mres[1] = pDb->ExecuteSql(L"DELETE FROM entries", false);
	mres[2] = pDb->ExecuteSql(L"DELETE FROM resource_entries", false);

	mres[3] = pDb->CommitTransaction();

	for(int i = 0; i < 4; i++) {
		if (IsFailed(mres[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(mres[i]));

			delete pDb;
			return mres[i];
		}
	}
	return E_SUCCESS;
}

result NotesManager::RemoveNote(Note *val) const {
	return RemoveNote(val->GetEntryId());
}

result NotesManager::RemoveNote(int entry_id) const {
	if (entry_id < 0)
		return E_INVALID_ARG;

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, true);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	String id = L""; res = id.Append(entry_id);
	if (IsFailed(res)) {
		delete pDb;
		return res;
	}

	result mres[4];
	mres[0] = pDb->BeginTransaction();

	mres[1] = pDb->ExecuteSql(L"DELETE FROM entries WHERE entry_id=" + id, false);
	mres[2] = pDb->ExecuteSql(L"DELETE FROM resource_entries WHERE entry_id=" + id, false);

	mres[3] = pDb->CommitTransaction();

	for(int i = 0; i < 4; i++) {
		if (IsFailed(mres[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(mres[i]));

			delete pDb;
			return mres[i];
		}
	}
	return E_SUCCESS;
}

LinkedListT<Note *> *NotesManager::GetNotesN(SortType sorting, SortOrder order, NoteType type_filter, FilterType filter_mode, const String &filter) const {
	LinkedListT<Note *> *pNotes = new LinkedListT<Note *>;

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, false);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pNotes;
		delete pDb;
		SetLastResult(res);

		return null;
	}

	for (int i = 1; i > -1; i--) {
		String query = L"SELECT entries.entry_id,entries.type,entries.timestamp,entries.marked,entries.title,entries.text, CASE "
		"WHEN (entries.type = 2) OR (entries.type = 3) THEN (SELECT res_path FROM resource_entries WHERE resource_entries.entry_id = entries.entry_id) "
		"ELSE '' "
		"END FROM entries WHERE (entries.marked = ";
		res = query.Append(i);
		res = query.Append(L") ");

		if (!filter.IsEmpty()) {
			if (filter_mode == FILTER_BY_TITLE) {
				res = query.Append(L"AND (UPPER(entries.title) LIKE UPPER(?)) ");
			} else {
				res = query.Append(L"AND (UPPER(entries.text) LIKE UPPER(?)) ");
			}
		}
		if (type_filter != NOTE_TYPE_ALL) {
			res = query.Append(L"AND (entries.type = ");
			res = query.Append((int)type_filter);
			res = query.Append(") ");
		}
		if (sorting == SORT_BY_DATE) {
			res = query.Append(L"ORDER BY entries.timestamp ");
		} else if (sorting == SORT_BY_TITLE) {
			res = query.Append(L"ORDER BY entries.title ");
		} else {
			res = query.Append(L"ORDER BY entries.type ");
		}
		res = query.Append(order == SORT_ORDER_ASCENDING ? L"ASC" : L"DESC");
		//'res' here can only be OUT_OF_MEMORY, thus check cumulatively
		if (IsFailed(res)) {
			AppLogException("Failed to construct query string, error: [%s]", GetErrorMessage(res));

			delete pNotes;
			delete pDb;
			SetLastResult(res);

			return null;
		}

		DbStatement *pStmt = pDb->CreateStatementN(query);
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to initialize query statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pNotes;
			delete pDb;
			SetLastResult(res);

			return null;
		}

		if (!filter.IsEmpty()) {
			String esc = L"%";
			res = esc.Append(filter);
			res = esc.Append('%');

			if (IsFailed(res)) {
				delete pStmt;
				delete pNotes;
				delete pDb;
				SetLastResult(res);

				return null;
			}

			if (!filter.IsEmpty()) {
				res = pStmt->BindString(0, esc);
			}

			if (IsFailed(res)) {
				AppLogException("Failed to bind transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

				delete pStmt;
				delete pNotes;
				delete pDb;
				SetLastResult(res);

				return null;
			}
		}

		DbEnumerator *pEnum = pDb->ExecuteStatementN(*pStmt);
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to execute query statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pStmt;
			delete pNotes;
			delete pDb;
			SetLastResult(res);

			return null;
		}

		if (pEnum) {
			while (!IsFailed(pEnum->MoveNext())) {
				Note *pNote = new Note;

				int entry_id = -1;
				int type = -1;
				long long timestamp = 0;
				int marked = 0;
				String title;
				String text;

				result gres[6];
				gres[0] = pEnum->GetIntAt(0, entry_id);
				gres[1] = pEnum->GetIntAt(1, type);
				gres[2] = pEnum->GetInt64At(2, timestamp);
				gres[3] = pEnum->GetIntAt(3, marked);
				gres[4] = pEnum->GetStringAt(4, title);
				gres[5] = pEnum->GetStringAt(5, text);

				for(int i = 0; i < 6; i++) {
					if (IsFailed(gres[i])) {
						AppLogException("Failed to retrieve query data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(gres[i]));

						delete pNote;
						delete pEnum;
						delete pStmt;
						delete pNotes;
						delete pDb;
						SetLastResult(gres[i]);

						return null;
					}
				}
				pNote->Construct((NoteType)type);
				pNote->SetEntryID(entry_id);
				pNote->SetSerialized(true);
				pNote->SetDate(timestamp);
				pNote->SetMarked(marked);
				pNote->SetTitle(title);
				pNote->SetText(text);

				if (type != NOTE_TYPE_TEXT) {
					String res_path;
					res = pEnum->GetStringAt(6, res_path);
					if (IsFailed(res)) {
						AppLogException("Failed to retrieve query data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

						delete pNote;
						delete pEnum;
						delete pStmt;
						delete pNotes;
						delete pDb;
						SetLastResult(res);

						return null;
					}
					pNote->SetResourcePath(res_path);
				}

				res = pNotes->Add(pNote);
				if (IsFailed(res)) {
					delete pNote;
					delete pEnum;
					delete pStmt;
					delete pNotes;
					delete pDb;
					SetLastResult(res);

					return null;
				}
			}
			delete pEnum;
			delete pStmt;
		}
	}

	delete pDb;
	return pNotes;
}
