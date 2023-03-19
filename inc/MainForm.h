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
#ifndef _MAINFORM_H_
#define _MAINFORM_H_

#include "BaseForm.h"
#include "CachingNotesManager.h"

class MainForm: public BaseForm, public IScrollPanelEventListener, public ICustomItemEventListener {
public:
	MainForm(void);
	virtual ~MainForm(void);

	virtual result Construct(void);

private:
	static const int ID_LIST_FORMAT_BITMAP = 500;
	static const int ID_LIST_FORMAT_DATE = 501;
	static const int ID_LIST_FORMAT_TITLE = 502;

	static const int ID_LIST_HEADER_FORMAT_BITMAP = 503;
	static const int ID_LIST_HEADER_FORMAT_TITLE = 504;

	static const int ID_SELECT_STORAGE_FILE = 505;

	static const int ID_CREATE_TEXT_NOTE = 506;
	static const int ID_EDIT_TEXT_NOTE = 507;

	bool CheckControls(void) const;
	String SortTypeToString(SortType type) const;
	result LoadNotes(void);
	result UpdateOptionMenu(void);
	result SwitchTab(void);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual void DialogCallback(int taskId, BaseForm *sender, DialogResult ret, void *dataN);

	result OnKeypadSearchClicked(const Control &src);
	result OnKeypadClearClicked(const Control &src);

	result OnOptionKeyClicked(const Control &src);
	result OnOptionSortByDateClicked(const Control &src);
	result OnOptionSortByTitleClicked(const Control &src);
	result OnOptionSortByTypeClicked(const Control &src);
	result OnOptionSearchByTitleClicked(const Control &src);
	result OnOptionSearchByTextClicked(const Control &src);
	result OnOptionChangeStorageClicked(const Control &src);

	result OnTabAllClicked(const Control &src);
	result OnTabTextClicked(const Control &src);
	result OnTabPhotoClicked(const Control &src);
	result OnTabAudioClicked(const Control &src);

	result OnLeftSoftkeyClicked(const Control &src);
	result OnRightSoftkeyClicked(const Control &src);

	virtual void OnOverlayControlCreated(const Control &source) {}
	virtual void OnOverlayControlOpened(const Control &source) {}
	virtual void OnOverlayControlClosed(const Control &source) { __pNotesList->SetFocus(); }
	virtual void OnOtherControlSelected(const Control &source) {}

	virtual void OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status);
	virtual void OnItemStateChanged(const Control &source, int index, int itemId, int elementId, ItemStatus status) {}

	DEF_ACTION(ID_OVERLAY_KEYPAD_CLEAR, 100);
	DEF_ACTION(ID_OVERLAY_KEYPAD_SEARCH, 101);

	DEF_ACTION(ID_OPTION_KEY_CLICKED, 102);
	DEF_ACTION(ID_OPTION_SORT_BY_DATE_CLICKED, 103);
	DEF_ACTION(ID_OPTION_SORT_BY_TITLE_CLICKED, 104);
	DEF_ACTION(ID_OPTION_SORT_BY_TYPE_CLICKED, 105);
	DEF_ACTION(ID_OPTION_SEARCH_BY_TITLE_CLICKED, 106);
	DEF_ACTION(ID_OPTION_SEARCH_BY_TEXT_CLICKED, 107);
	DEF_ACTION(ID_OPTION_CHANGE_STORAGE, 108);

	DEF_ACTION(ID_TAB_ALL_CLICKED, 109);
	DEF_ACTION(ID_TAB_TEXT_CLICKED, 110);
	DEF_ACTION(ID_TAB_PHOTO_CLICKED, 111);
	DEF_ACTION(ID_TAB_AUDIO_CLICKED, 112);

	DEF_ACTION(ID_SOFTKEY0_CLICKED, 113);
	DEF_ACTION(ID_SOFTKEY1_CLICKED, 114);

	ScrollPanel *__pMainPanel;
	EditField *__pSearchField;
	CustomList *__pNotesList;
	OptionMenu *__pOptionMenu;
	Tab *__pTabPanel;

	CustomListItemFormat *__pNotesListItemFormat;
	CustomListItemFormat *__pNotesListSortingHeaderFormat;
	CachingNotesManager *__pNotesManager;
	NoteType __currentTab;
	SortType __currentSorting;
	SortOrder __currentSortOrder;
	FilterType __currentFilterMode;

	Bitmap *__pHeaderIconAsc;
	Bitmap *__pHeaderIconDesc;
	Bitmap *__pTextIcon;
	Bitmap *__pAudioIcon;
	Bitmap *__pPhotoIcon;
	Bitmap *__pMarkIcon;
};

#endif
