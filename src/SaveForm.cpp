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
#include <FIo.h>

#include "SaveForm.h"
#include "FormManager.h"

using namespace Osp::Io;
using namespace Osp::Base::Utility;

SaveForm::SaveForm(CallbackInfo cbInfo, const String &startingPath) {
	__pFilenameField = null;
	__pDirListCaption = null;
	__pDirList = null;
	__pOptionMenu = null;

	__pDirListItemFormat = null;
	__pCurrentDir = null;
	__pCurrentDirList = null;
	__pCurrentFileList = null;
	__pNavigationHistory = null;

	__cbInfo = cbInfo;
	__startingPath = startingPath;
}

SaveForm::~SaveForm(void) {
	if (__pOptionMenu) delete __pOptionMenu;
	if (__pDirListItemFormat) delete __pDirListItemFormat;
	if (__pCurrentDir) delete __pCurrentDir;
	if (__pCurrentDirList) delete __pCurrentDirList;
	if (__pCurrentFileList) delete __pCurrentFileList;
	if (__pNavigationHistory) delete __pNavigationHistory;
}

result SaveForm::Construct(void) {
	if (!__cbInfo.callbackHandler) {
		AppLogException("Attempt to construct dialog form without callback handler, callback will not be sent!");
	}

	result res = Form::Construct(L"IDF_SAVEFORM");
	if (IsFailed(res)) {
		AppLogException("Failed to construct form's XML structure, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pFilenameField = static_cast<EditField*>(GetControl(L"IDC_SAVEFORM_FILENAME_FIELD"));
	__pDirListCaption = static_cast<Label*>(GetControl(L"IDC_SAVEFORM_DIRLIST_CAPTION"));
	__pDirList = static_cast<CustomList*>(GetControl(L"IDC_SAVEFORM_DIRLIST"));

	if (!CheckControls()) {
		AppLogException("Failed to initialize custom form structure");
		return E_INIT_FAILED;
	}

	__pOptionMenu = new OptionMenu;
	res = __pOptionMenu->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct option menu, error: [%s]", GetErrorMessage(res));
		return E_INIT_FAILED;
	}
	__pOptionMenu->AddItem(GetString(L"SAVEFORM_OPTIONMENU_CREATE_FOLDER"),ID_OPTION_CREATE_FOLDER_CLICKED);

	__pDirListItemFormat = new CustomListItemFormat;
	res = __pDirListItemFormat->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct directory list item format, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pDirListItemFormat->AddElement(ID_DIRLIST_FORMAT_BITMAP, Rectangle(0,0,90,90));
	__pDirListItemFormat->AddElement(ID_DIRLIST_FORMAT_NAME, Rectangle(95,10,380,37));
	__pDirListItemFormat->AddElement(ID_DIRLIST_FORMAT_ELEMENTS, Rectangle(95,57,380,37), 22, Color::COLOR_GREY, Color::COLOR_GREY);

	__pCurrentDir = new String(L"/");
	__pCurrentDirList = new LinkedList;
	__pCurrentFileList = new LinkedList;
	__pNavigationHistory = new Stack;

	return E_SUCCESS;
}

bool SaveForm::CheckControls(void) const {
	if (__pFilenameField && __pDirList) {
		return true;
	} else return false;
}

int SaveForm::GetDirectoryElementsCount(const String &dir) const {
	Directory *pDir = new Directory;
	result res = pDir->Construct(dir);
	if (IsFailed(res)) {
		AppLogException("Failed to construct directory object for path [%S], error: [%s]", dir.GetPointer(), GetErrorMessage(res));

		delete pDir;

		SetLastResult(res);
		return -1;
	}

	DirEnumerator *pDirEnum = pDir->ReadN();
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to construct directory enumerator for path [%S], error: [%s]", dir.GetPointer(), GetErrorMessage(res));

		delete pDir;

		SetLastResult(res);
		return -1;
	}

	int count = 0;
	while (!IsFailed(pDirEnum->MoveNext())) {
		DirEntry entry = pDirEnum->GetCurrentDirEntry();
		if (entry.GetName().Equals(String(L".")) || entry.GetName().Equals(String(L".."))) {
			continue;
		}
		count++;
	}

	delete pDir;
	delete pDirEnum;

	return count;
}

result SaveForm::FillRootDirList(void) {
	__pCurrentDirList->RemoveAll(true);
	__pCurrentFileList->RemoveAll(true);
	__pDirList->RemoveAllItems();

	Bitmap *pFolder = GetBitmapN(L"folder_icon.png");
	result res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire icons for directory list items, error [%s]", GetErrorMessage(res));
		return res;
	}

	Bitmap *pFolderMC = GetBitmapN(L"folder_icon_mc.png");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire icons for directory list items, error [%s]", GetErrorMessage(res));
		delete pFolder;
		return res;
	}

	Bitmap *pFolderIntM = GetBitmapN(L"folder_icon_intm.png");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire icons for directory list items, error [%s]", GetErrorMessage(res));
		delete pFolder;
		delete pFolderMC;
		return res;
	}

	String *paths[3] = {
		new String(L"/Home/"),
		new String(L"/Media/Others/"),
		new String(L"/Storagecard/Media/Others/")
	};

	String path_names[3] = {
		GetString(L"SAVEFORM_DIR_ENTRY_APPFOLDER"),
		GetString(L"SAVEFORM_DIR_ENTRY_INTMEM"),
		GetString(L"SAVEFORM_DIR_ENTRY_STORAGECARD")
	};

	for(int i = 0; i < 3; i++) {
		CustomListItem *pItem = new CustomListItem;
		res = pItem->Construct(90);
		if (IsFailed(res)) {
			AppLogException("Failed to construct directory list item, error: [%s]", GetErrorMessage(res));
			delete pItem;
			break;
		}

		pItem->SetItemFormat(*__pDirListItemFormat);
		if (i == 2) {
			pItem->SetElement(ID_DIRLIST_FORMAT_BITMAP, *pFolderMC, pFolderMC);
		} else if (i == 1) {
			pItem->SetElement(ID_DIRLIST_FORMAT_BITMAP, *pFolderIntM, pFolderIntM);
		} else {
			pItem->SetElement(ID_DIRLIST_FORMAT_BITMAP, *pFolder, pFolder);
		}
		pItem->SetElement(ID_DIRLIST_FORMAT_NAME, path_names[i]);

		int count = GetDirectoryElementsCount(*paths[i]);
		if (count < 0) {
			AppLogException("Failed to enumerate directory elements for path [%S], error: [%s]", paths[i]->GetPointer(), GetErrorMessage(res));
			res = GetLastResult();

			delete pItem;
			break;
		} else if (count > 0) {
			String elements = GetString(L"SAVEFORM_DIR_ENTRY_INFO"); elements.Append(count);
			pItem->SetElement(ID_DIRLIST_FORMAT_ELEMENTS, elements);
		} else {
			pItem->SetElement(ID_DIRLIST_FORMAT_ELEMENTS, GetString(L"SAVEFORM_DIR_ENTRY_INFO_EMPTY"));
		}

		__pDirList->AddItem(*pItem, i);
		__pCurrentDirList->Add(*paths[i]);
	}

	delete pFolder;
	delete pFolderMC;
	delete pFolderIntM;

	SetFormStyle(GetFormStyle() & ~FORM_STYLE_OPTIONKEY);
	RefreshForm();
	return res;
}

result SaveForm::FillDirList(const String &dir) {
	if (dir.Equals(String(L"/"))) {
		return FillRootDirList();
	}

	__pCurrentDirList->RemoveAll(true);
	__pCurrentFileList->RemoveAll(true);
	__pDirList->RemoveAllItems();

	Bitmap *pFile = GetBitmapN(L"file_icon.png");
	result res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire icons for directory list items, error [%s]", GetErrorMessage(res));
		return res;
	}

	Bitmap *pFolder = GetBitmapN(L"folder_icon.png");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire icons for directory list items, error [%s]", GetErrorMessage(res));
		delete pFile;
		return res;
	}

	Bitmap *pBack = GetBitmapN(L"dir_up_icon.png");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire icons for directory list items, error [%s]", GetErrorMessage(res));

		delete pFile;
		delete pFolder;

		return res;
	}

	Directory *pDir = new Directory;
	res = pDir->Construct(dir);
	if (IsFailed(res)) {
		AppLogException("Failed to construct directory object for path [%S], error: [%s]", dir.GetPointer(), GetErrorMessage(res));

		delete pFile;
		delete pFolder;
		delete pBack;
		delete pDir;

		return res;
	}

	DirEnumerator *pEnum = pDir->ReadN();
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to construct directory enumerator for path [%S], error: [%s]", dir.GetPointer(), GetErrorMessage(res));

		delete pFile;
		delete pFolder;
		delete pBack;
		delete pDir;

		return res;
	}

	int i = 0;
	int j = -2;
	while (!IsFailed(pEnum->MoveNext())) {
		DirEntry entry = pEnum->GetCurrentDirEntry();
		if (entry.GetName().Equals(String(L"."))) {
			continue;
		}

		CustomListItem *pItem = new CustomListItem;
		res = pItem->Construct(90);
		if (IsFailed(res)) {
			AppLogException("Failed to construct directory list item, error: [%s]", GetErrorMessage(res));
			delete pItem;
			break;
		}

		pItem->SetItemFormat(*__pDirListItemFormat);

		if (entry.GetName().Equals(String(L".."))) {
			pItem->SetElement(ID_DIRLIST_FORMAT_NAME, GetString(L"SAVEFORM_NAVIGATE_BACK"));
			pItem->SetElement(ID_DIRLIST_FORMAT_BITMAP, *pBack, pBack);
			pItem->SetElement(ID_DIRLIST_FORMAT_ELEMENTS, *(static_cast<const String*>(__pNavigationHistory->Peek())));

			__pDirList->AddItem(*pItem, -1);
		} else if (entry.IsDirectory()) {
			pItem->SetElement(ID_DIRLIST_FORMAT_NAME, entry.GetName());
			pItem->SetElement(ID_DIRLIST_FORMAT_BITMAP, *pFolder, pFolder);

			String path = dir;
			path.Append(entry.GetName());
			path.Append('/');

			int count = GetDirectoryElementsCount(path);
			if (count < 0) {
				AppLogException("Failed to enumerate directory elements for path [%S], error: [%s]", path.GetPointer(), GetErrorMessage(res));
				res = GetLastResult();

				delete pItem;
				break;
			} else if (count > 0) {
				String elements = GetString(L"SAVEFORM_DIR_ENTRY_INFO"); elements.Append(count);
				pItem->SetElement(ID_DIRLIST_FORMAT_ELEMENTS, elements);
			} else {
				pItem->SetElement(ID_DIRLIST_FORMAT_ELEMENTS, GetString(L"SAVEFORM_DIR_ENTRY_INFO_EMPTY"));
			}

			__pCurrentDirList->Add(*(new String(path)));
			__pDirList->AddItem(*pItem, i++);
		} else {
			pItem->SetElement(ID_DIRLIST_FORMAT_NAME, entry.GetName());
			pItem->SetElement(ID_DIRLIST_FORMAT_BITMAP, *pFile, pFile);

			String info;
			info.Format(60, GetString(L"SAVEFORM_FILE_ENTRY_INFO_FORMAT").GetPointer(), (entry.GetFileSize() / 1024), entry.GetDateTime().ToString().GetPointer());

			pItem->SetElement(ID_DIRLIST_FORMAT_ELEMENTS, info);

			__pCurrentFileList->Add(*(new String(entry.GetName())));
			__pDirList->AddItem(*pItem, j--);
		}
	}

	delete pFile;
	delete pFolder;
	delete pBack;
	delete pEnum;
	delete pDir;

	if (dir.StartsWith(L"/Home", 0)) {
		SetFormStyle(GetFormStyle() | FORM_STYLE_OPTIONKEY);
	}
	RefreshForm();
	return res;
}

result SaveForm::NavigateToPath(const String &path) {
	if (path.IsEmpty()) {
		return E_INVALID_ARG;
	}

	delete __pCurrentDir;
	__pNavigationHistory->RemoveAll(true);

	StringTokenizer strTok(path, L"/");

	String token;
	String *dir = new String(L"/");
	if (path.EndsWith(L"/")) {
		while (strTok.HasMoreTokens()) {
			__pNavigationHistory->Push(*(new String(*dir)));

			strTok.GetNextToken(token);
			token.Append('/');

			dir->Append(token);
		}
	} else {
		int count = strTok.GetTokenCount();
		for(int i = 0; i < count - 1; i++) {
			__pNavigationHistory->Push(*(new String(*dir)));

			strTok.GetNextToken(token);
			token.Append('/');

			dir->Append(token);
		}
		strTok.GetNextToken(token);
		__pFilenameField->SetText(token);
	}

	__pCurrentDir = dir;
	return FillDirList(*__pCurrentDir);
}

result SaveForm::OnLeftSoftkeyClicked(const Control &src) {
	if (__pFilenameField->GetTextLength() == 0) {
		ShowMessageBox(GetString(L"SAVEFORM_MBOX_DIR_TITLE"), GetString(L"SAVEFORM_MBOX_FILE_MSG"),MSGBOX_STYLE_OK);
		return E_SUCCESS;
	}

	result res = E_SUCCESS;
	if (__cbInfo.callbackHandler) {
		String *pCbData = new String(*__pCurrentDir);
		res = pCbData->Append(__pFilenameField->GetText());
		if (IsFailed(res)) {
			AppLogException("Failed to construct callback data, error: [%s]", GetErrorMessage(res));
			delete pCbData;
			return res;
		}

		if (File::IsFileExist(*pCbData)) {
			int mres = ShowMessageBox(GetString(L"SAVEFORM_MBOX_TITLE"), GetString(L"SAVEFORM_MBOX_TEXT"), MSGBOX_STYLE_YESNO);

			res = GetLastResult();
			if (IsFailed(res)) {
				AppLogException("Failed to construct message box, selected file will be overwritten, error: [%s]", GetErrorMessage(res));
			} else if (mres != MSGBOX_RESULT_YES) {
				delete pCbData;
				return E_SUCCESS;
			}
		}

		__cbInfo.callbackHandler->DialogCallback(__cbInfo.taskID, this, DIALOG_RESULT_OK, (void*)pCbData);
	}
	res = FormManager::SetPreviousFormActive(true);
	if (IsFailed(res)) {
		AppLogException("Failed to switch back from file selection dialog, error: [%s]", GetErrorMessage(res));
	}
	return res;
}

result SaveForm::OnRightSoftkeyClicked(const Control &src) {
	if (__cbInfo.callbackHandler) {
		__cbInfo.callbackHandler->DialogCallback(__cbInfo.taskID, this, DIALOG_RESULT_CANCEL, null);
	}
	return FormManager::SetPreviousFormActive(true);
}

result SaveForm::OnOptionKeyClicked(const Control &src) {
	if (__pOptionMenu) {
		result res = __pOptionMenu->SetShowState(true);
		if (IsFailed(res)) return res;
		return __pOptionMenu->Show();
	} else return E_INVALID_STATE;
}

result SaveForm::OnCreateFolderClicked(const Control &src) {
	if (__pCurrentDir) {
		result res = ShowInputBox(CALLBACK(ID_INPUT_DIRECTORY_NAME), KEYPAD_MODE_ALPHA, GetString(L"SAVEFORM_INPUT_DIRNAME_TITLE"), GetString(L"SAVEFORM_INPUT_DIRNAME_MSG"),
				GetString(L"SAVEFORM_INPUT_DIRNAME_ACCEPT_BUTTON"), GetString(L"SAVEFORM_CANCEL_BUTTON"));
		if (IsFailed(res)) {
			AppLogException("Failed to request user input, error: [%s]", GetErrorMessage(res));
		}
		return res;
	} else return E_INVALID_STATE;
}

result SaveForm::Initialize(void) {
	SetTitleText(GetString(L"SAVEFORM_TITLE"), ALIGNMENT_CENTER);

	RegisterAction(ID_SOFTKEY0_CLICKED, HANDLER(SaveForm::OnLeftSoftkeyClicked));
	RegisterAction(ID_SOFTKEY1_CLICKED, HANDLER(SaveForm::OnRightSoftkeyClicked));
	SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY0_CLICKED);
	SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY1_CLICKED);
	AddSoftkeyActionListener(SOFTKEY_0, *this);
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	SetSoftkeyText(SOFTKEY_0, GetString(L"SAVEFORM_ACCEPT_BUTTON"));
	SetSoftkeyText(SOFTKEY_1, GetString(L"SAVEFORM_CANCEL_BUTTON"));

	RegisterAction(ID_OPTION_KEY_CLICKED, HANDLER(SaveForm::OnOptionKeyClicked));
	RegisterAction(ID_OPTION_CREATE_FOLDER_CLICKED, HANDLER(SaveForm::OnCreateFolderClicked));
	__pOptionMenu->AddActionEventListener(*this);
	SetOptionkeyActionId(ID_OPTION_KEY_CLICKED);
	AddOptionkeyActionListener(*this);

	__pFilenameField->SetTitleText(GetString(L"SAVEFORM_FILENAME_FIELD_TITLE"));

	__pDirListCaption->SetText(GetString(L"SAVEFORM_DIRLIST_TITLE"));
	__pDirList->AddCustomItemEventListener(*this);

	if (!__startingPath.IsEmpty()) {
		return NavigateToPath(__startingPath);
	} else return FillRootDirList();
}

result SaveForm::Terminate(void) {
	return E_SUCCESS;
}

void SaveForm::OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status) {
	const String *path = null;
	if (itemId < -1) {
		int real_id = -(itemId + 2);
		__pFilenameField->SetText(String(*(static_cast<String*>(__pCurrentFileList->GetAt(real_id)))));

		RefreshForm();
		return;
	} else if (itemId == -1) {
		delete __pCurrentDir;
		path = static_cast<const String*>(__pNavigationHistory->Pop());
	} else {
		__pNavigationHistory->Push(*__pCurrentDir);
		path = new String(*(static_cast<String*>(__pCurrentDirList->GetAt(itemId))));
	}

	result res = FillDirList(*path);

	__pCurrentDir = path;

	if (IsFailed(res)) {
		AppLogException("Failed to navigate to directory, error: [%s]", GetErrorMessage(res));
	}
}

void SaveForm::DialogCallback(int taskId, BaseForm *sender, DialogResult ret, void *dataN) {
	if (taskId == ID_INPUT_DIRECTORY_NAME && ret == DIALOG_RESULT_OK && dataN) {
		String *pCbData = (String*)dataN;
		if (pCbData->IsEmpty()) {
			ShowMessageBox(GetString(L"SAVEFORM_MBOX_DIR_TITLE"), GetString(L"SAVEFORM_MBOX_DIR_MSG"),MSGBOX_STYLE_OK);
		}

		String *dir = new String(*__pCurrentDir);
		dir->Append(*pCbData);
		dir->Append('/');

		Directory::Create(*dir, true);
		FillDirList(*__pCurrentDir);

		delete pCbData;
		delete dir;
	}
}
