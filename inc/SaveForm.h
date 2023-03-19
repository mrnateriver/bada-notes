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
#ifndef _SAVEFORM_H_
#define _SAVEFORM_H_

#include "BaseForm.h"

using namespace Osp::Base::Collection;

class SaveForm: public BaseForm, public ICustomItemEventListener
{
public:
	SaveForm(CallbackInfo cbInfo, const String &startingPath = L"");
	virtual ~SaveForm(void);

	virtual result Construct(void);

private:
	static const int ID_DIRLIST_FORMAT_BITMAP = 500;
	static const int ID_DIRLIST_FORMAT_NAME = 501;
	static const int ID_DIRLIST_FORMAT_ELEMENTS = 502;

	static const int ID_INPUT_DIRECTORY_NAME = 503;

	bool CheckControls(void) const;
	int GetDirectoryElementsCount(const String &dir) const;
	result FillRootDirList(void);
	result FillDirList(const String &dir);
	result NavigateToPath(const String &path);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual void DialogCallback(int taskId, BaseForm *sender, DialogResult ret, void *dataN);

	result OnLeftSoftkeyClicked(const Control &src);
	result OnRightSoftkeyClicked(const Control &src);

	result OnOptionKeyClicked(const Control &src);
	result OnCreateFolderClicked(const Control &src);

	virtual void OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status);
	virtual void OnItemStateChanged(const Control &source, int index, int itemId, int elementId, ItemStatus status) {}

	DEF_ACTION(ID_SOFTKEY0_CLICKED, 100);
	DEF_ACTION(ID_SOFTKEY1_CLICKED, 101);

	DEF_ACTION(ID_OPTION_KEY_CLICKED, 102);
	DEF_ACTION(ID_OPTION_CREATE_FOLDER_CLICKED, 103);

	EditField *__pFilenameField;
	Label *__pDirListCaption;
	CustomList *__pDirList;
	OptionMenu *__pOptionMenu;

	CustomListItemFormat *__pDirListItemFormat;
	const String *__pCurrentDir;
	LinkedList *__pCurrentDirList;
	LinkedList *__pCurrentFileList;
	Stack *__pNavigationHistory;

	CallbackInfo __cbInfo;
	String __startingPath;
};

#endif
