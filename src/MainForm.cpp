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

#include "FormManager.h"
#include "MainForm.h"
#include "SaveForm.h"
#include "TextNoteForm.h"

using namespace Osp::App;

MainForm::MainForm(void) {
	__pMainPanel = null;
	__pSearchField = null;
	__pNotesList = null;
	__pOptionMenu = null;
	__pTabPanel = null;

	__pNotesListItemFormat = null;
	__pNotesListSortingHeaderFormat = null;
	__pNotesManager = null;
	__currentTab = NOTE_TYPE_ALL;
	__currentSorting = SORT_BY_DATE;
	__currentSortOrder = SORT_ORDER_DESCENDING;
	__currentFilterMode = FILTER_BY_TITLE;

	__pHeaderIconAsc = null;
	__pHeaderIconDesc = null;
	__pTextIcon = null;
	__pAudioIcon = null;
	__pPhotoIcon = null;
	__pMarkIcon = null;
}

MainForm::~MainForm(void) {
	if (__pOptionMenu) delete __pOptionMenu;
	if (__pNotesListItemFormat) delete __pNotesListItemFormat;
	if (__pNotesListSortingHeaderFormat) delete __pNotesListSortingHeaderFormat;
	if (__pNotesManager) delete __pNotesManager;

	if (__pHeaderIconAsc) delete __pHeaderIconAsc;
	if (__pHeaderIconDesc) delete __pHeaderIconDesc;
	if (__pTextIcon) delete __pTextIcon;
	if (__pAudioIcon) delete __pAudioIcon;
	if (__pPhotoIcon) delete __pPhotoIcon;
	if (__pMarkIcon) delete __pMarkIcon;
}

result MainForm::Construct() {
	result res = Form::Construct(L"IDF_MAINFORM");
	if (IsFailed(res)) {
		AppLogException("Failed to construct form's XML structure, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pTabPanel = GetTab();
	if (__pTabPanel) {
		__pTabPanel->SetEditModeEnabled(true);
	}

	__pMainPanel = static_cast<ScrollPanel*>(GetControl(L"IDC_MAINFORM_MAIN_SCROLLPANEL"));
	if (__pMainPanel) {
		__pSearchField = static_cast<EditField*>(__pMainPanel->GetControl(L"IDPC_MAINFORM_SEARCH_FIELD"));
		__pNotesList = static_cast<CustomList*>(__pMainPanel->GetControl(L"IDPC_MAINFORM_LIST"));
	}

	if (!CheckControls()) {
		AppLogException("Failed to initialize custom form structure");
		return E_INIT_FAILED;
	}

	__pOptionMenu = new OptionMenu;
	res = __pOptionMenu->Construct();

	if (IsFailed(res)) {
		AppLogException("Failed to construct option menu, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pNotesListItemFormat = new CustomListItemFormat;
	res = __pNotesListItemFormat->Construct();

	if (IsFailed(res)) {
		AppLogException("Failed to construct custom list item format, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pNotesListItemFormat->AddElement(ID_LIST_FORMAT_BITMAP, Rectangle(11,11,32,32));
	__pNotesListItemFormat->AddElement(ID_LIST_FORMAT_DATE, Rectangle(52,17,428,30), 21);
	__pNotesListItemFormat->AddElement(ID_LIST_FORMAT_TITLE, Rectangle(15,47,450,45), 31);

	__pNotesListSortingHeaderFormat = new CustomListItemFormat;
	res = __pNotesListSortingHeaderFormat->Construct();

	if (IsFailed(res)) {
		AppLogException("Failed to construct custom list header format, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pNotesListSortingHeaderFormat->AddElement(ID_LIST_HEADER_FORMAT_BITMAP, Rectangle(0,0,48,48));
	__pNotesListSortingHeaderFormat->AddElement(ID_LIST_HEADER_FORMAT_TITLE, Rectangle(48,3,432,45));

	__pNotesManager = new CachingNotesManager;

	return E_SUCCESS;
}

bool MainForm::CheckControls(void) const {
	if (__pMainPanel && __pSearchField && __pNotesList && __pTabPanel) {
		return true;
	} else return false;
}

String MainForm::SortTypeToString(SortType type) const {
	if (type == SORT_BY_DATE) {
		return GetString(L"SORT_TYPE_DATE");
	} else if (type == SORT_BY_TITLE) {
		return GetString(L"SORT_TYPE_TITLE");
	} else {
		return GetString(L"SORT_TYPE_TYPE");
	}
}

result MainForm::LoadNotes(void) {
	__pNotesList->RemoveAllItems();

	LinkedListT<Note *> *pNotes = __pNotesManager->GetNotesN(__currentSorting, __currentSortOrder, __currentTab,
															 __currentFilterMode, __pSearchField->GetText());
	result res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to get notes list, error: [%s]", GetErrorMessage(res));
		return res;
	}

	CustomListItem *pHeaderItem = new CustomListItem;
	res = pHeaderItem->Construct(48);
	if (IsFailed(res)) {
		AppLogException("Failed to construct notes list item, error: [%s]", GetErrorMessage(res));

		delete pHeaderItem;
		delete pNotes;

		return res;
	}

	Bitmap *header_icon = __currentSortOrder == SORT_ORDER_ASCENDING ? __pHeaderIconAsc : __pHeaderIconDesc;

	pHeaderItem->SetItemFormat(*__pNotesListSortingHeaderFormat);

	pHeaderItem->SetElement(ID_LIST_HEADER_FORMAT_BITMAP, *header_icon, header_icon);
	pHeaderItem->SetElement(ID_LIST_HEADER_FORMAT_TITLE, GetString(L"MAINFORM_NOTES_LIST_HEADER_TITLE") + SortTypeToString(__currentSorting));

	__pNotesList->AddItem(*pHeaderItem, 1);

	int i = 0;
	IEnumeratorT<Note *> *pEnum = pNotes->GetEnumeratorN();
	if (pEnum) {
		while(!IsFailed(pEnum->MoveNext())) {
			Note *pNote; pEnum->GetCurrent(pNote);

			CustomListItem *pItem = new CustomListItem;
			res = pItem->Construct(90);
			if (IsFailed(res)) {
				AppLogException("Failed to construct notes list item, error: [%s]", GetErrorMessage(res));

				delete pItem;
				delete pNotes;

				return res;
			}

			pItem->SetItemFormat(*__pNotesListItemFormat);

			pItem->SetElement(ID_LIST_FORMAT_DATE, GetLocaleSpecificDatetime(pNote->GetDate()));

			res = GetLastResult();
			if (IsFailed(res)) {
				AppLogException("Failed to acquire locale specific datetime, error: [%s]", GetErrorMessage(res));

				delete pItem;
				delete pNotes;

				return res;
			}

			if (pNote->GetMarked()) {
				pItem->SetElement(ID_LIST_FORMAT_BITMAP,*__pMarkIcon,__pMarkIcon);
			} else {
				if (pNote->GetType() == NOTE_TYPE_TEXT) {
					pItem->SetElement(ID_LIST_FORMAT_BITMAP,*__pTextIcon,__pTextIcon);
				} else if (pNote->GetType() == NOTE_TYPE_AUDIO) {
					pItem->SetElement(ID_LIST_FORMAT_BITMAP,*__pAudioIcon,__pAudioIcon);
				} else if (pNote->GetType() == NOTE_TYPE_PHOTO) {
					pItem->SetElement(ID_LIST_FORMAT_BITMAP,*__pPhotoIcon,__pPhotoIcon);
				}
			}

			pItem->SetElement(ID_LIST_FORMAT_TITLE,pNote->GetTitle());

			__pNotesList->AddItem(*pItem, i + 2);
			i++;
		}
	}

	RefreshForm();

	delete pEnum;
	delete pNotes;

	return E_SUCCESS;
}

result MainForm::UpdateOptionMenu(void) {
	if (__pOptionMenu) {
		result res = E_SUCCESS;
		if (__currentTab != NOTE_TYPE_ALL && __pOptionMenu->GetSubItemCount(0) > 2) {
			res = __pOptionMenu->RemoveSubItemAt(0, __pOptionMenu->GetSubItemIndexFromActionId(ID_OPTION_SORT_BY_TYPE_CLICKED));
			if (IsFailed(res)) {
				AppLogException("Failed to remove sort by type subitem of option menu, error: [%s]", GetErrorMessage(res));
			}
		} else if (__currentTab == NOTE_TYPE_ALL && __pOptionMenu->GetSubItemCount(0) < 3) {
			__pOptionMenu->AddSubItem(0, GetString(L"MAINFORM_OPTIONMENU_SORT_BY_TYPE"), ID_OPTION_SORT_BY_TYPE_CLICKED);
		}
		return res;
	} else return E_INVALID_STATE;
}

result MainForm::SwitchTab(void) {
	if (__currentTab != NOTE_TYPE_ALL && __currentSorting == SORT_BY_TYPE) {
		__currentSorting = SORT_BY_DATE;
	}

	result res = LoadNotes();
	if (IsFailed(res)) {
		return res;
	} else return UpdateOptionMenu();
}

result MainForm::OnKeypadSearchClicked(const Control &src) {
	__pMainPanel->CloseOverlayWindow();

	result res = __pSearchField->Draw();
	if (IsFailed(res))
		return res;

	res = __pSearchField->Show();
	if (IsFailed(res))
		return res;

	return LoadNotes();
}

result MainForm::OnKeypadClearClicked(const Control &src) {
	__pSearchField->Clear();
	return OnKeypadSearchClicked(src);
}

result MainForm::OnOptionKeyClicked(const Control &src) {
	if (__pOptionMenu) {
		result res = __pOptionMenu->SetShowState(true);
		if (IsFailed(res)) return res;
		return __pOptionMenu->Show();
	} else return E_INVALID_STATE;
}

result MainForm::OnOptionSortByDateClicked(const Control &src) {
	__currentSorting = SORT_BY_DATE;
	return LoadNotes();
}

result MainForm::OnOptionSortByTitleClicked(const Control &src) {
	__currentSorting = SORT_BY_TITLE;
	return LoadNotes();
}

result MainForm::OnOptionSortByTypeClicked(const Control &src) {
	__currentSorting = SORT_BY_TYPE;
	return LoadNotes();
}

result MainForm::OnOptionSearchByTitleClicked(const Control &src) {
	__currentFilterMode = FILTER_BY_TITLE;

	String sGuide = GetString(L"MAINFORM_SEARCH_FIELD_GUIDE");
	sGuide.Append(__currentFilterMode == FILTER_BY_TITLE ? GetString(L"MAINFORM_SEARCH_FIELD_GUIDE_BY_TITLE") : GetString(L"MAINFORM_SEARCH_FIELD_GUIDE_BY_TEXT"));
	__pSearchField->SetGuideText(sGuide);

	return LoadNotes();
}

result MainForm::OnOptionSearchByTextClicked(const Control &src) {
	__currentFilterMode = FILTER_BY_TEXT;

	String sGuide = GetString(L"MAINFORM_SEARCH_FIELD_GUIDE");
	sGuide.Append(__currentFilterMode == FILTER_BY_TITLE ? GetString(L"MAINFORM_SEARCH_FIELD_GUIDE_BY_TITLE") : GetString(L"MAINFORM_SEARCH_FIELD_GUIDE_BY_TEXT"));
	__pSearchField->SetGuideText(sGuide);

	return LoadNotes();
}

result MainForm::OnOptionChangeStorageClicked(const Control &src) {
	SaveForm *pSaveForm = new SaveForm(CALLBACK(ID_SELECT_STORAGE_FILE), __pNotesManager->GetPath());
	result res = pSaveForm->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct file selection dialog, error: [%s]", GetErrorMessage(res));
		delete pSaveForm;
		return res;
	}

	res = FormManager::SetActiveForm(pSaveForm);
	if (IsFailed(res)) {
		AppLogException("Failed to switch to file selection dialog, error: [%s]", GetErrorMessage(res));
		delete pSaveForm;
	}

	return res;
}

result MainForm::OnTabAllClicked(const Control &src) {
	if (__currentTab != NOTE_TYPE_ALL) {
		__currentTab = NOTE_TYPE_ALL;
		return SwitchTab();
	} else return E_SUCCESS;
}

result MainForm::OnTabTextClicked(const Control &src) {
	if (__currentTab != NOTE_TYPE_TEXT) {
		__currentTab = NOTE_TYPE_TEXT;
		return SwitchTab();
	} else return E_SUCCESS;
}

result MainForm::OnTabPhotoClicked(const Control &src) {
	if (__currentTab != NOTE_TYPE_PHOTO) {
		__currentTab = NOTE_TYPE_PHOTO;
		return SwitchTab();
	} else return E_SUCCESS;
}

result MainForm::OnTabAudioClicked(const Control &src) {
	if (__currentTab != NOTE_TYPE_AUDIO) {
		__currentTab = NOTE_TYPE_AUDIO;
		return SwitchTab();
	} else return E_SUCCESS;
}

result MainForm::OnLeftSoftkeyClicked(const Control &src) {
	return E_SUCCESS;
}

result MainForm::OnRightSoftkeyClicked(const Control &src) {
	TextNoteForm *pNoteForm = new TextNoteForm(CALLBACK(ID_CREATE_TEXT_NOTE));
	result res = pNoteForm->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct text note creation form, error: [%s]", GetErrorMessage(res));
		delete pNoteForm;
		return res;
	}

	res = FormManager::SetActiveForm(pNoteForm);
	if (IsFailed(res)) {
		AppLogException("Failed to switch to text note creation form, error: [%s]", GetErrorMessage(res));
		delete pNoteForm;
	}

	return res;
}

result MainForm::Initialize() {
	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_SORT_BY"), 1);
	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_SEARCH_BY"), 2);
	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_SOURCE"), ID_OPTION_CHANGE_STORAGE);
	__pOptionMenu->AddSubItem(0, GetString(L"MAINFORM_OPTIONMENU_SORT_BY_TITLE"), ID_OPTION_SORT_BY_TITLE_CLICKED);
	__pOptionMenu->AddSubItem(0, GetString(L"MAINFORM_OPTIONMENU_SORT_BY_DATE"), ID_OPTION_SORT_BY_DATE_CLICKED);
	__pOptionMenu->AddSubItem(0, GetString(L"MAINFORM_OPTIONMENU_SORT_BY_TYPE"), ID_OPTION_SORT_BY_TYPE_CLICKED);
	__pOptionMenu->AddSubItem(1, GetString(L"MAINFORM_OPTIONMENU_SEARCH_BY_TITLE"), ID_OPTION_SEARCH_BY_TITLE_CLICKED);
	__pOptionMenu->AddSubItem(1, GetString(L"MAINFORM_OPTIONMENU_SEARCH_BY_TEXT"), ID_OPTION_SEARCH_BY_TEXT_CLICKED);

	SetSoftkeyText(SOFTKEY_1, GetString(L"MAINFORM_SOFTKEY_ADD"));

	RegisterAction(ID_OPTION_KEY_CLICKED, HANDLER(MainForm::OnOptionKeyClicked));
	RegisterAction(ID_OPTION_SORT_BY_DATE_CLICKED, HANDLER(MainForm::OnOptionSortByDateClicked));
	RegisterAction(ID_OPTION_SORT_BY_TITLE_CLICKED, HANDLER(MainForm::OnOptionSortByTitleClicked));
	RegisterAction(ID_OPTION_SORT_BY_TYPE_CLICKED, HANDLER(MainForm::OnOptionSortByTypeClicked));
	RegisterAction(ID_OPTION_SEARCH_BY_TITLE_CLICKED, HANDLER(MainForm::OnOptionSearchByTitleClicked));
	RegisterAction(ID_OPTION_SEARCH_BY_TEXT_CLICKED, HANDLER(MainForm::OnOptionSearchByTextClicked));
	RegisterAction(ID_OPTION_CHANGE_STORAGE, HANDLER(MainForm::OnOptionChangeStorageClicked));
	__pOptionMenu->AddActionEventListener(*this);
	SetOptionkeyActionId(ID_OPTION_KEY_CLICKED);
	AddOptionkeyActionListener(*this);

	__pSearchField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_RIGHT, GetString(L"MAINFORM_SEARCH_KEYPAD_CLEAR"), ID_OVERLAY_KEYPAD_CLEAR);
	__pSearchField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_LEFT, GetString(L"MAINFORM_SEARCH_KEYPAD_SEARCH"), ID_OVERLAY_KEYPAD_SEARCH);

	RegisterAction(ID_OVERLAY_KEYPAD_SEARCH, HANDLER(MainForm::OnKeypadSearchClicked));
	RegisterAction(ID_OVERLAY_KEYPAD_CLEAR, HANDLER(MainForm::OnKeypadClearClicked));
	__pSearchField->AddActionEventListener(*this);
	__pSearchField->AddScrollPanelEventListener(*this);

	RegisterAction(ID_TAB_ALL_CLICKED, HANDLER(MainForm::OnTabAllClicked));
	RegisterAction(ID_TAB_TEXT_CLICKED, HANDLER(MainForm::OnTabTextClicked));
	RegisterAction(ID_TAB_PHOTO_CLICKED, HANDLER(MainForm::OnTabPhotoClicked));
	RegisterAction(ID_TAB_AUDIO_CLICKED, HANDLER(MainForm::OnTabAudioClicked));
	__pTabPanel->AddActionEventListener(*this);

	__pNotesList->AddCustomItemEventListener(*this);

	RegisterAction(ID_SOFTKEY1_CLICKED, HANDLER(MainForm::OnRightSoftkeyClicked));
	SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY1_CLICKED);
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	__pHeaderIconAsc = GetBitmapN(L"sort_asc_icon.png");
	__pHeaderIconDesc = GetBitmapN(L"sort_desc_icon.png");
	__pTextIcon = GetBitmapN(L"small_text_icon.png");
	__pAudioIcon = GetBitmapN(L"small_audio_icon.png");
	__pPhotoIcon = GetBitmapN(L"small_photo_icon.png");
	__pMarkIcon = GetBitmapN(L"small_mark_icon.png");

	result res = GetLastResult();
	if (IsFailed(res) || (!__pHeaderIconAsc || !__pHeaderIconDesc || !__pTextIcon || !__pAudioIcon || !__pPhotoIcon || !__pMarkIcon)) {
		AppLogException("Failed to acquire necessary icons, error [%s]", GetErrorMessage(res));
		return res;
	}

	AppRegistry *appReg = Application::GetInstance()->GetAppRegistry();

	String prefs[4] = {
		L"MAINFORM_TAB_FIRST_INDEX",
		L"MAINFORM_TAB_SECOND_INDEX",
		L"MAINFORM_TAB_THIRD_INDEX",
		L"MAINFORM_TAB_FOURTH_INDEX"
	};
	String strings[4] = {
		"MAINFORM_TAB_TITLE_ALL",
		"MAINFORM_TAB_TITLE_TEXT",
		"MAINFORM_TAB_TITLE_PHOTO",
		"MAINFORM_TAB_TITLE_AUDIO"
	};
	Bitmap *bitmaps[4] = {
		null,
		__pTextIcon,
		__pPhotoIcon,
		__pAudioIcon
	};

	int index = -1;
	for(int i = 0; i < 4; i++) {
		res = appReg->Get(prefs[i], index);
		if (IsFailed(res)) {
			index = i;
			res = appReg->Add(prefs[i], index);

			if (IsFailed(res)) {
				AppLogException("Failed to add registry key for storing tab order, error [%s]", GetErrorMessage(res));
				return res;
			}
		}
		Bitmap *cur_bitmap = bitmaps[index];
		if (cur_bitmap) {
			__pTabPanel->AddItem(*cur_bitmap, GetString(strings[index]), ID_TAB_ALL_CLICKED + index);
		} else {
			__pTabPanel->AddItem(GetString(strings[index]), ID_TAB_ALL_CLICKED + index);
		}
	}
	__currentTab = (NoteType)(__pTabPanel->GetItemActionIdAt(__pTabPanel->GetSelectedItemIndex()) - ID_TAB_ALL_CLICKED);

	int sel_tab = 0;
	res = appReg->Get(L"MAINFORM_SELECTED_TAB", sel_tab);
	if (IsFailed(res)) {
		res = appReg->Add(L"MAINFORM_SELECTED_TAB", sel_tab);
		if (IsFailed(res)) {
			AppLogException("Failed to add registry key for storing selected tab. Selected tab will not be saved, error [%s]", GetErrorMessage(res));
		}
	}
	__pTabPanel->SetSelectedItem(sel_tab);
	__currentTab = (NoteType)(__pTabPanel->GetItemActionIdAt(sel_tab) - ID_TAB_ALL_CLICKED);

	int sorting = 0;
	res = appReg->Get(L"MAINFORM_NOTES_LIST_SORTING_MODE", sorting);
	if (IsFailed(res)) {
		res = appReg->Add(L"MAINFORM_NOTES_LIST_SORTING_MODE", sorting);
		if (IsFailed(res)) {
			AppLogException("Failed to add registry key for storing sorting mode. Sorting mode will not be saved, error [%s]", GetErrorMessage(res));
		}
	}
	__currentSorting = (SortType)sorting;

	int sort_order = 1;
	res = appReg->Get(L"MAINFORM_NOTES_LIST_SORTING_ORDER", sort_order);
	if (IsFailed(res)) {
		res = appReg->Add(L"MAINFORM_NOTES_LIST_SORTING_ORDER", 1);
		if (IsFailed(res)) {
			AppLogException("Failed to add registry key for storing sorting order. Sorting order will not be saved, error [%s]", GetErrorMessage(res));
		}
	}
	__currentSortOrder = (SortOrder)sort_order;

	int filter_mode = 0;
	res = appReg->Get(L"MAINFORM_NOTES_LIST_FILTER_MODE", filter_mode);
	if (IsFailed(res)) {
		res = appReg->Add(L"MAINFORM_NOTES_LIST_FILTER_MODE", 0);
		if (IsFailed(res)) {
			AppLogException("Failed to add registry key for storing filtering mode. Filtering mode will not be saved, error [%s]", GetErrorMessage(res));
		}
	}
	__currentFilterMode = (FilterType)filter_mode;

	String sGuide = GetString(L"MAINFORM_SEARCH_FIELD_GUIDE");
	sGuide.Append(__currentFilterMode == FILTER_BY_TITLE ? GetString(L"MAINFORM_SEARCH_FIELD_GUIDE_BY_TITLE") : GetString(L"MAINFORM_SEARCH_FIELD_GUIDE_BY_TEXT"));
	__pSearchField->SetGuideText(sGuide);

	//+TBR
	String dataPath = L"/Home/notes.bin";
	res = appReg->Get(L"ALLNOTES_STORAGE_FILE", dataPath);
	if (IsFailed(res)) {
		res = appReg->Add(L"ALLNOTES_STORAGE_FILE", L"/Home/notes.bin");
		if (IsFailed(res)) {
			AppLogException("Failed to add registry key for storing storage file path, using default, error [%s]", GetErrorMessage(res));
		}
	}
	__pNotesManager->Construct(dataPath);
	//-TBR

	appReg->Save();

	res = SwitchTab();
	if (IsFailed(res)) {
		AppLogException("Failed to fill notes list, error: [%s]", GetErrorMessage(res));
	}
	return res;
}

result MainForm::Terminate() {
	AppRegistry *appReg = Application::GetInstance()->GetAppRegistry();

	String prefs[4] = {
		L"MAINFORM_TAB_FIRST_INDEX",
		L"MAINFORM_TAB_SECOND_INDEX",
		L"MAINFORM_TAB_THIRD_INDEX",
		L"MAINFORM_TAB_FOURTH_INDEX"
	};
	result res = E_SUCCESS;
	for(int i = 0; i < __pTabPanel->GetItemCount(); i++) {
		res = appReg->Set(prefs[i], __pTabPanel->GetItemActionIdAt(i) - ID_TAB_ALL_CLICKED);
		if (IsFailed(res)) {
			AppLogException("Failed to save tab order to registry, error: [%s]", GetErrorMessage(res));
			break;
		}
	}

	res = appReg->Set(L"MAINFORM_SELECTED_TAB", (int)__pTabPanel->GetSelectedItemIndex());
	if (IsFailed(res)) {
		AppLogException("Failed to save selected tab to registry, error: [%s]", GetErrorMessage(res));
	}

	res = appReg->Set(L"MAINFORM_NOTES_LIST_SORTING_MODE", (int)__currentSorting);
	if (IsFailed(res)) {
		AppLogException("Failed to save sorting mode to registry, error: [%s]", GetErrorMessage(res));
	}

	res = appReg->Set(L"MAINFORM_NOTES_LIST_SORTING_ORDER", (int)__currentSortOrder);
	if (IsFailed(res)) {
		AppLogException("Failed to save sorting order to registry, error: [%s]", GetErrorMessage(res));
	}

	res = appReg->Set(L"MAINFORM_NOTES_LIST_FILTER_MODE", (int)__currentFilterMode);
	if (IsFailed(res)) {
		AppLogException("Failed to save filtering mode to registry, error: [%s]", GetErrorMessage(res));
	}

	appReg->Save();
	return res;
}

void MainForm::DialogCallback(int taskId, BaseForm *sender, DialogResult ret, void *dataN) {
	result res = E_SUCCESS;

	Note *pCbData = null;
	if (dataN) {
		pCbData = (Note*)dataN;
	}

	if (taskId == ID_CREATE_TEXT_NOTE && ret == DIALOG_RESULT_OK) {
		if (pCbData) {
			res = __pNotesManager->AddNote(pCbData);
			if (IsFailed(res)) {
				AppLogException("Failed to add created note to the database, error: [%s]", GetErrorMessage(res));
				delete pCbData;
				return;
			}
		}
	} else if (taskId == ID_EDIT_TEXT_NOTE && ret == DIALOG_RESULT_OK) {
		if (pCbData) {
			res = __pNotesManager->UpdateNote(pCbData);
			if (IsFailed(res)) {
				AppLogException("Failed to update note in the database, error: [%s]", GetErrorMessage(res));
				delete pCbData;
				return;
			}
		}
	}
	res = LoadNotes();
	if (IsFailed(res)) {
		AppLogException("Failed to fill notes list, error: [%s]", GetErrorMessage(res));
	}
}

void MainForm::OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status) {
	if (itemId == 1) {
		if (__currentSortOrder == SORT_ORDER_ASCENDING) {
			__currentSortOrder = SORT_ORDER_DESCENDING;
		} else {
			__currentSortOrder = SORT_ORDER_ASCENDING;
		}
		result res = LoadNotes();
		if (IsFailed(res)) {
			AppLogException("Failed to load notes after switching sorting order, error: [%s]", GetErrorMessage(res));
		}
	} else {
		itemId = itemId - 2;
		Note *pNote = __pNotesManager->GetNote(itemId);

		TextNoteForm *pNoteForm = new TextNoteForm(CALLBACK(ID_EDIT_TEXT_NOTE), pNote);
		result res = pNoteForm->Construct();
		if (IsFailed(res)) {
			AppLogException("Failed to construct text note creation form, error: [%s]", GetErrorMessage(res));
			delete pNoteForm;
			return;
		}

		res = FormManager::SetActiveForm(pNoteForm);
		if (IsFailed(res)) {
			AppLogException("Failed to switch to text note creation form, error: [%s]", GetErrorMessage(res));
			delete pNoteForm;
		}
	}
}
