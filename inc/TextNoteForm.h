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
#ifndef _TEXTNOTEFORM_H_
#define _TEXTNOTEFORM_H_

#include "BaseForm.h"
#include "Note.h"

class TextNoteForm: public BaseForm
{
public:
	TextNoteForm(CallbackInfo cbInfo, Note *note = null);
	virtual ~TextNoteForm(void);

	virtual result Construct(void);

private:
	bool CheckControls(void) const;
	result UpdateMarkButton(void);

	virtual result Initialize(void);
	virtual result Terminate(void);

	result OnLeftSoftkeyClicked(const Control &src);
	result OnRightSoftkeyClicked(const Control &src);

	result OnMarkButtonClicked(const Control &src);

	DEF_ACTION(ID_SOFTKEY0_CLICKED, 100);
	DEF_ACTION(ID_SOFTKEY1_CLICKED, 101);

	DEF_ACTION(ID_MARK_BUTTON_CLICKED, 102);

	EditField *__pTitleField;
	Button *__pMarkButton;
	Label *__pTextCaption;
	EditArea *__pTextArea;
	OptionMenu *__pOptionMenu;
	Label *__pTimeCaption;
	Label *__pTimeLabel;

	Note *__pNote;
	bool __isMarked;

	CallbackInfo __cbInfo;
};

#endif
