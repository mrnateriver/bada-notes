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
#include "AllNotes.h"
#include "FormManager.h"
#include "MainForm.h"

AllNotes::AllNotes() {
}

AllNotes::~AllNotes() {
}

Application *AllNotes::CreateInstance(void) {
	return new AllNotes();
}

bool AllNotes::OnAppInitializing(AppRegistry &appRegistry) {
	PowerManager::SetScreenEventListener(*this);

	MainForm *pMainForm = new MainForm();
	result res = pMainForm->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct main application form, error: [%s]", GetErrorMessage(res));
		return false;
	}

	res = FormManager::SetActiveForm(pMainForm);
	if (IsFailed(res)) {
		AppLogException("Failed to switch to main application form, error: [%s]", GetErrorMessage(res));
		//return false;
	}

	return true;
}

bool AllNotes::OnAppTerminating(AppRegistry &appRegistry, bool forcedTermination) {
	// TODO:
	// Deallocate resources allocated by this application for termination.
	// The application's permanent data and context can be saved via appRegistry.
	return true;
}

void AllNotes::OnForeground(void) {
	// TODO:
	// Start or resume drawing when the application is moved to the foreground.
}

void AllNotes::OnBackground(void) {
	// TODO:
	// Stop drawing when the application is moved to the background.
}

void AllNotes::OnLowMemory(void) {
	// TODO:
	// Free unused resources or close the application.
}

void AllNotes::OnBatteryLevelChanged(BatteryLevel batteryLevel) {
	if (batteryLevel == BATTERY_CRITICAL) {
		MessageBox *pMsg = new MessageBox;
		pMsg->Construct(L"Warning", L"Your battery level is critical! Note that some features of this application may cause high power consumption, like camera, audio recorder etc."
		" So, if you may need that power in the nearest future, we recommend you stop using this application until you fully charge your battery.", MSGBOX_STYLE_OK);

		int res = -1;
		pMsg->ShowAndWait(res);

		delete pMsg;
	}
}

void AllNotes::OnScreenOn(void) {
	// TODO:
	// Get the released resources or resume the operations that were paused or stopped in OnScreenOff().
}

void AllNotes::OnScreenOff(void) {
	// TODO:
	//  Unless there is a strong reason to do otherwise, release resources (such as 3D, media, and sensors) to allow the device to enter the sleep mode to save the battery.
	// Invoking a lengthy asynchronous method within this listener method can be risky, because it is not guaranteed to invoke a callback before the device enters the sleep mode.
	// Similarly, do not perform lengthy operations in this listener method. Any operation must be a quick one.
}
