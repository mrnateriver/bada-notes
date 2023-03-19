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
#include "TextNoteForm.h"
#include "FormManager.h"

TextNoteForm::TextNoteForm(CallbackInfo cbInfo, Note *note) {
	__pTitleField = null;
	__pMarkButton = null;
	__pTextCaption = null;
	__pTextArea = null;
	__pOptionMenu = null;
	__pTimeCaption = null;
	__pTimeLabel = null;

	__pNote = note;
	__isMarked = false;

	__cbInfo = cbInfo;
}

TextNoteForm::~TextNoteForm(void) {
	if (__pOptionMenu) delete __pOptionMenu;
}

result TextNoteForm::Construct(void) {
	result res = Form::Construct(L"IDF_TEXTNOTE_FORM");
	if (IsFailed(res)) {
		AppLogException("Failed to construct form's XML structure, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pTitleField = static_cast<EditField*>(GetControl(L"IDC_TEXTNOTE_FORM_TITLEFIELD"));
	__pMarkButton = static_cast<Button*>(GetControl(L"IDC_TEXTNOTE_FORM_MARK_BUTTON"));
	__pTextCaption = static_cast<Label*>(GetControl(L"IDC_TEXTNOTE_FORM_CAPTION"));
	__pTextArea = static_cast<EditArea*>(GetControl(L"IDC_TEXTNOTE_FORM_EDITAREA"));
	__pTimeCaption = static_cast<Label*>(GetControl(L"IDC_TEXTNOTE_FORM_TIMECAPTION"));
	__pTimeLabel = static_cast<Label*>(GetControl(L"IDC_TEXTNOTE_FORM_TIMELABEL"));

	if (!CheckControls()) {
		AppLogException("Failed to initialize custom form structure");
		return E_INIT_FAILED;
	}

	__pOptionMenu = new OptionMenu;
	res = __pOptionMenu->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct option menu, error [%s]", GetErrorMessage(res));
	}

	return res;
}

bool TextNoteForm::CheckControls(void) const {
	if (__pTitleField && __pMarkButton && __pTextCaption && __pTextArea
			&& __pTimeCaption && __pTimeLabel) {
		return true;
	} else return false;
}

result TextNoteForm::UpdateMarkButton(void) {
	String sNormal = L"button_marked_unpressed.png";
	String sPressed = L"button_marked_pressed.png";
	if (!__isMarked) {
		sNormal = L"button_unmarked_unpressed.png";
		sPressed = L"button_unmarked_pressed.png";
	}

	Bitmap *pNormal = GetBitmapN(sNormal);
	result res1 = GetLastResult();
	Bitmap *pPressed = GetBitmapN(sPressed);
	result res2 = GetLastResult();

	if (IsFailed(res1) || IsFailed(res2)) {
		AppLogException("Failed to update mark button icon, error1: [%s], error2: [%s]", GetErrorMessage(res1), GetErrorMessage(res2));
		if (pNormal) delete pNormal;
		if (pPressed) delete pPressed;
	}

	__pMarkButton->SetNormalBackgroundBitmap(*pNormal);
	__pMarkButton->SetPressedBackgroundBitmap(*pPressed);

	delete pNormal;
	delete pPressed;

	return RefreshForm();
}

result TextNoteForm::OnLeftSoftkeyClicked(const Control &src) {
	if (__pTextArea->GetTextLength() == 0) {
		ShowMessageBox(GetString(L"SAVEFORM_MBOX_DIR_TITLE"), GetString(L"TEXTNOTE_FORM_MBOX_MSG"),MSGBOX_STYLE_OK);
		return E_SUCCESS;
	}

	if (__cbInfo.callbackHandler) {
		Note *pCbData = __pNote;
		if (!pCbData) {
			pCbData = new Note;
			pCbData->Construct(NOTE_TYPE_TEXT);
		}
		pCbData->SetDate(GetCurrentTimeInUTCUnixTicks() / 1000);
		pCbData->SetMarked(__isMarked);
		pCbData->SetText(__pTextArea->GetText());

		String title = L"";
		if (__pTitleField->GetTextLength() == 0) {
			if (__pTextArea->GetTextLength() >= 57) {
				__pTextArea->GetText().SubString(0, 57, title);
				title.Append(L"...");
			} else {
				title = __pTextArea->GetText();
			}
		} else {
			title = __pTitleField->GetText();
		}
		pCbData->SetTitle(title);

		pCbData->SetSerialized(false);
		__cbInfo.callbackHandler->DialogCallback(__cbInfo.taskID, this, DIALOG_RESULT_OK, (void*)pCbData);
	}
	return FormManager::SetPreviousFormActive(true);
}

result TextNoteForm::OnRightSoftkeyClicked(const Control &src) {
	if (__cbInfo.callbackHandler) {
		__cbInfo.callbackHandler->DialogCallback(__cbInfo.taskID, this, DIALOG_RESULT_CANCEL, null);
	}
	return FormManager::SetPreviousFormActive(true);
}

result TextNoteForm::OnMarkButtonClicked(const Control &src) {
	__isMarked = !__isMarked;
	return UpdateMarkButton();
}

result TextNoteForm::Initialize(void) {
	__pTitleField->SetTitleText(GetString(L"TEXTNOTE_FORM_TITLEFIELD_TITLE"));
	__pTextCaption->SetText(GetString(L"TEXTNOTE_FORM_TEXT_CAPTION"));
	__pTimeCaption->SetText(GetString(L"TEXTNOTE_FORM_TIME_CAPTION"));

	SetSoftkeyText(SOFTKEY_1, GetString(L"TEXTNOTE_FORM_SOFTKEY_CANCEL"));

	result res = E_SUCCESS;
	if (__pNote) {
		//edit mode
		SetTitleText(GetString(L"TEXTNOTE_FORM_TITLE_EDIT"), ALIGNMENT_LEFT);
		SetSoftkeyText(SOFTKEY_0, GetString(L"TEXTNOTE_FORM_SOFTKEY_ACCEPT_EDIT"));

		__pTitleField->SetText(__pNote->GetTitle());
		__pTextArea->SetText(__pNote->GetText());
		__pTimeLabel->SetText(GetLocaleSpecificDatetime(__pNote->GetDate()));
		__isMarked = __pNote->GetMarked();

		res = UpdateMarkButton();
		if (IsFailed(res)) {
			AppLogException("Failed to initialize form from existing note object, error: [%s]", GetErrorMessage(res));
			return res;
		}
	} else {
		//create mode
		SetTitleText(GetString(L"TEXTNOTE_FORM_TITLE_NEW"), ALIGNMENT_LEFT);
		SetSoftkeyText(SOFTKEY_0, GetString(L"TEXTNOTE_FORM_SOFTKEY_ACCEPT_NEW"));

		__pTextArea->SetSize(460,450);
		__pTimeCaption->SetShowState(false);
		__pTimeLabel->SetShowState(false);
	}

	RegisterButtonPressEvent(__pMarkButton, ID_MARK_BUTTON_CLICKED, HANDLER(TextNoteForm::OnMarkButtonClicked));

	RegisterAction(ID_SOFTKEY0_CLICKED, HANDLER(TextNoteForm::OnLeftSoftkeyClicked));
	RegisterAction(ID_SOFTKEY1_CLICKED, HANDLER(TextNoteForm::OnRightSoftkeyClicked));
	SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY0_CLICKED);
	SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY1_CLICKED);
	AddSoftkeyActionListener(SOFTKEY_0, *this);
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	return E_SUCCESS;
}

result TextNoteForm::Terminate(void) {
	return E_SUCCESS;
}
