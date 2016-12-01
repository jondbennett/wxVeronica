/***************************************************************
 * Name:      VeronicaMain.h
 * Purpose:   Defines Application Frame
 * Author:    Jon Bennett (jon@jondbennett.com)
 * Created:   2016-11-18
 * Copyright: Jon Bennett (http://jondbennett.com)
 * License:
 **************************************************************/

#ifndef VERONICAMAIN_H
#define VERONICAMAIN_H

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "VeronicaApp.h"


#include <wx/button.h>
#include <wx/statline.h>
class VeronicaDialog: public wxDialog, public ITelemetry_ReceiveTarget
{
public:
	VeronicaDialog(wxDialog *dlg, const wxString& title);
	~VeronicaDialog();

protected:

	// Send / receive telemetry
	wxString m_serialPortName;
	int m_serialBaudRate;
	bool m_reportDeadSerial;

	CSerialComm m_serialComm;
	long m_lastCommData;
	bool m_commsAreUp;

	CTelemetry m_telemetry;
	int m_telemetryVersion;
	CCommandSender *m_commandSender;

	long m_timeToSendTelemetryVersion ;

	wxArrayString m_telemData;
	void startReception();
	void receiveTerm(int _index, const char *_value);
	void receiveChecksumCorrect();
	void receiveChecksumError();
	CTelemetryData_01 m_telemetryData_01;
	void sendTelemetryVersion(bool _force);

	void showNakDialog(int _tag, double _value, int _reason);

	enum
	{
		idTimerPoll = 1000,
		idTimerUpdate,

		idForceDoorButton,
		idForceLightButton,
		idOpenSettingsButton,
	};

	wxStaticText *m_ctrlGPSStatus;
	wxStaticText *m_ctrlCommStatus;
	wxStaticText *m_ctrlCurrentTime;

	wxStaticText *m_ctrlMorningLightTimes;
	wxStaticText *m_ctrlDoorTimes;
	wxStaticText *m_ctrlEveningLightTimes;

	wxStaticText *m_ctrlDoorStatus;
	wxStaticText *m_ctrlLightStatus;

	wxButton *m_ctrlForceDoorButton;
	wxButton *m_ctrlForceLightButton;

	wxButton *m_openSettings;

	bool m_oldGPSLockStatus;

	void OnClose(wxCloseEvent& event);
	void OnQuit(wxCommandEvent& event);

	void OnButton_ForceDoor(wxCommandEvent &_e);
	void OnButton_ForceLight(wxCommandEvent &_e);

	void OnButton_OpenSettings(wxCommandEvent &_e);
	void LoadSettings();

	wxTimer m_updateTimer;
	void onTimer_updateTimer(wxTimerEvent &_e);

	wxTimer m_pollTimer;
	void onTimer_pollTimer(wxTimerEvent &_e);

	void CreateControls();
	void UpdateControls();

	wxString formatDoubleTime(double _t);

	DECLARE_EVENT_TABLE()
};

#endif // VERONICAMAIN_H
