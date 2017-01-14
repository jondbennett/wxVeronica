/***************************************************************
 * Name:      VeronicaMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Jon Bennett (jon@jondbennett.com)
 * Created:   2016-11-18
 * Copyright: Jon Bennett (http://jondbennett.com)
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/sound.h>
#include <wx/config.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include <time.h>

#include "../../../GaryCooper/ICommInterface.h"
#include "../../../GaryCooper/Telemetry.h"
#include "../../../GaryCooper/TelemetryTags.h"

#include "SerialComm.h"
#include "TelemetryData01.h"
#include "SettingsDialog.h"
#include "ChangeNotifier.h"
#include "ConfigKeys.h"
#include "CommandSender.h"
#include "TimeDisplayFormat.h"
#include "Freezer.h"

#include "VeronicaMain.h"

// Serial port stuff
#define POLL_TIMER_MS	(50)
#define UPDATE_TIMER_MS (1000)
#define COMM_DATA_TIMEOUT	(15)
#define SEND_PROTOCOL_VERSION_TIME	(15L)

#define EFFECTIVELY_EQUAL(v1, v2) ((fabs(v1-v2) < 0.02))
//#define EFFECTIVELY_EQUAL(v1, v2) (false)

#define BOXBORDER (5)
#define CONTROLBORDER (3)

// Notification sounds
CChangeNotifier g_notifyCoopCommStatus("Warning_01.wav", "Assurance_01.wav");
CChangeNotifier g_notifyCoopGPSLockStatus("Warning_02.wav", "Assurance_02.wav");
CChangeNotifier g_notifyCoopGPSError("Warning_03.wav", "Assurance_03.wav");
CChangeNotifier g_notifyCoopDoorError("Warning_04.wav", "Assurance_04.wav");
CChangeNotifier g_notifyCoopControllerError("Warning_05.wav", "Assurance_05.wav");
CChangeNotifier g_notifyCoopDoorStatus("Notice_01.wav", "Notice_02.wav");
CChangeNotifier g_notifyCoopLightStatus("Notice_03.wav", "Notice_04.wav");
static time_t initialNotificationDelay = time(0L) + COMM_DATA_TIMEOUT;

BEGIN_EVENT_TABLE(VeronicaDialog, wxDialog)
	EVT_CLOSE(VeronicaDialog::OnClose)

	EVT_BUTTON(idForceDoorButton, VeronicaDialog::OnButton_ForceDoor)
	EVT_BUTTON(idForceLightButton, VeronicaDialog::OnButton_ForceLight)
	EVT_BUTTON(idOpenSettingsButton, VeronicaDialog::OnButton_OpenSettings)

	EVT_TIMER (idTimerUpdate, VeronicaDialog::onTimer_updateTimer)
	EVT_TIMER (idTimerPoll, VeronicaDialog::onTimer_pollTimer)

END_EVENT_TABLE()

VeronicaDialog::VeronicaDialog(wxDialog *dlg, const wxString &title)
	: wxDialog(dlg, -1, title, wxDefaultPosition, wxDefaultSize,wxDEFAULT_DIALOG_STYLE | wxMINIMIZE_BOX)
{
	CreateControls();

	m_serialBaudRate = TELEMETRY_BAUD_RATE;
	m_reportDeadSerial = true;

	m_lastCommData = time(0);
	m_telemetryVersion = TELEMETRY_VERSION_INVALID;

	m_oldGPSLockStatus = false;
	m_commsAreUp = false;

	m_telemetry.setInterfaces(&m_serialComm, this);
	m_commandSender = new CCommandSender(&m_telemetry);
	m_timeToSendTelemetryVersion = 0L;

	m_updateTimer.SetOwner(this, idTimerUpdate);
	m_updateTimer.Start(UPDATE_TIMER_MS, false);

	m_pollTimer.SetOwner(this, idTimerPoll);
	m_pollTimer.Start(POLL_TIMER_MS, true);

	LoadSettings();

	bool minimized = false;
	wxConfig::Get()->Read(config_key_start_minimized, &minimized, false);
	if(minimized)
		Iconize(true);
}

VeronicaDialog::~VeronicaDialog()
{
	delete m_commandSender;
	m_serialComm.close();
}

void VeronicaDialog::CreateControls()
{
	// Create the main box sizer
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	// Reusable variables
	wxStaticText *label;

	//////////////////////////////////////////////////////////////////////////
	// Comm and GPS status indicator sizer
	{
		wxStaticBoxSizer *commBoxSizer = new wxStaticBoxSizer(
			wxVERTICAL,
			this,
			_("Comm Status"));
		mainSizer->Add(commBoxSizer,
					   wxSizerFlags().
					   Border(wxTOP | wxLEFT | wxRIGHT, BOXBORDER).
					   Expand());
		{
			wxFlexGridSizer* commStatusSizer;
			commStatusSizer = new wxFlexGridSizer(2, wxSize(5, 5));
			commStatusSizer->AddGrowableCol(1);
			commBoxSizer->Add(commStatusSizer, wxSizerFlags().Border(wxALL, 10));

			// Comm Status
			label = new wxStaticText(this, -1, _("Comm Status:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT);
			commStatusSizer->Add(label, 0, wxALIGN_RIGHT);

			m_ctrlCommStatus = new wxStaticText(this, -1, _("Unknown"),
												wxDefaultPosition, wxDefaultSize,
												wxALIGN_RIGHT);

			commStatusSizer->Add(m_ctrlCommStatus, 0, wxALIGN_RIGHT);

			// GPS Status
			label = new wxStaticText(this, -1, _("GPS Status:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT);
			commStatusSizer->Add(label, 0, wxALIGN_RIGHT);

			m_ctrlGPSStatus = new wxStaticText(this, -1, _("Unknown"),
											   wxDefaultPosition, wxDefaultSize,
											   wxALIGN_RIGHT);
			commStatusSizer->Add(m_ctrlGPSStatus, 0, wxALIGN_RIGHT);

			// GPS Status
			label = new wxStaticText(this, -1, _("Current Time:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT);
			commStatusSizer->Add(label, 0, wxALIGN_RIGHT);

			m_ctrlCurrentTime = new wxStaticText(this, -1, _("Unknown"),
												 wxDefaultPosition, wxDefaultSize,
												 wxALIGN_RIGHT);
			commStatusSizer->Add(m_ctrlCurrentTime, 0, wxALIGN_RIGHT);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Schedule sizer
	{
		wxStaticBoxSizer *scheduleBoxSizer = new wxStaticBoxSizer(
			wxVERTICAL,
			this,
			_("Door and Light Schedule"));
		mainSizer->Add(scheduleBoxSizer,
					   wxSizerFlags().
					   Border(wxTOP | wxLEFT | wxRIGHT, BOXBORDER).
					   Expand());
		{
			wxFlexGridSizer* scheduleSizer;
			scheduleSizer = new wxFlexGridSizer(2, wxSize(5, 5));
			scheduleSizer->AddGrowableCol(1);
			scheduleBoxSizer->Add(scheduleSizer, wxSizerFlags().Border(wxALL, 10));

			// Morning Light
			label = new wxStaticText(this, -1, _("Morning Light:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT);
			scheduleSizer->Add(label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

			m_ctrlMorningLightTimes = new wxStaticText(this, -1, _("XX:XX - XX:XX"),
					wxDefaultPosition, wxDefaultSize,
					wxALIGN_RIGHT);
			scheduleSizer->Add(m_ctrlMorningLightTimes, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

			// Door times
			label = new wxStaticText(this, -1, _("Door Open:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
			scheduleSizer->Add(label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

			m_ctrlDoorTimes = new wxStaticText(this, -1, _("XX:XX - XX:XX"),
											   wxDefaultPosition, wxDefaultSize,
											   wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
			scheduleSizer->Add(m_ctrlDoorTimes, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

			// Evening Light
			label = new wxStaticText(this, -1, _("Evening Light:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
			scheduleSizer->Add(label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

			m_ctrlEveningLightTimes = new wxStaticText(this, -1, _("XX:XX - XX:XX"),
					wxDefaultPosition, wxDefaultSize,
					wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
			scheduleSizer->Add(m_ctrlEveningLightTimes, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Door and light status sizer
	{
		wxStaticBoxSizer *commandBoxSizer = new wxStaticBoxSizer(
			wxVERTICAL,
			this,
			_("Commands"));
		mainSizer->Add(commandBoxSizer,
					   wxSizerFlags().
					   Border(wxTOP | wxLEFT | wxRIGHT, BOXBORDER).
					   Expand());

		{

			wxFlexGridSizer* doorLightStatusSizer;
			doorLightStatusSizer = new wxFlexGridSizer(3, wxSize(5, 5));
			doorLightStatusSizer->AddGrowableCol(1);
			commandBoxSizer->Add(doorLightStatusSizer, wxSizerFlags().Border(wxALL, 10));

			// Door Status
			label = new wxStaticText(this, -1, _("Door Status:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT);
			doorLightStatusSizer->Add(label, 0, wxALIGN_RIGHT);

			m_ctrlDoorStatus = new wxStaticText(this, -1, _("Unknown"),
												wxDefaultPosition, wxDefaultSize,
												wxALIGN_RIGHT);
			doorLightStatusSizer->Add(m_ctrlDoorStatus, 0, wxALIGN_RIGHT);

			m_ctrlForceDoorButton = new wxButton(this, idForceDoorButton, _("Open"));
			doorLightStatusSizer->Add(m_ctrlForceDoorButton, 0, wxALIGN_RIGHT);

			// Light Status
			label = new wxStaticText(this, -1, _("Light Status:"),
									 wxDefaultPosition, wxDefaultSize,
									 wxALIGN_RIGHT);
			doorLightStatusSizer->Add(label, 0, wxALIGN_RIGHT);

			m_ctrlLightStatus = new wxStaticText(this, -1, _("Unknown"),
												 wxDefaultPosition, wxDefaultSize,
												 wxALIGN_RIGHT);
			doorLightStatusSizer->Add(m_ctrlLightStatus, 0, wxALIGN_RIGHT);

			m_ctrlForceLightButton = new wxButton(this, idForceLightButton, _("On"));
			doorLightStatusSizer->Add(m_ctrlForceLightButton, 0, wxALIGN_RIGHT);
		}

		//////////////////////////////////////////////////////////////////////////
		// Buttons Sizer
		{
			wxFlexGridSizer* buttonsSizer;
			buttonsSizer = new wxFlexGridSizer(3, wxSize(5, 5));
			buttonsSizer->AddGrowableCol(1);
			mainSizer->Add(buttonsSizer, wxSizerFlags().Border(wxALL, 10));

			m_openSettings = new wxButton(this, idOpenSettingsButton, _("Settings..."));
			buttonsSizer->Add(m_openSettings, 0, wxALIGN_RIGHT);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Finish up
	this->SetSizer(mainSizer);
	this->Layout();
	mainSizer->Fit(this);
}

void VeronicaDialog::UpdateControls()
{
	CFreezer freezer(this);
	wxString labelText;

	// Comm status
	if(m_commsAreUp)
		m_ctrlCommStatus->SetLabel(_("OK"));
	else
		m_ctrlCommStatus->SetLabel(_("ERROR"));

	// GPS status
	if(m_telemetryData_01.m_GPSLocked)
	{
		labelText = wxString::Format(_("OK (%d)"), m_telemetryData_01.m_nSats);
		m_ctrlGPSStatus->SetLabel(labelText);
	}
	else
		m_ctrlGPSStatus->SetLabel(_("ERROR"));

	if((m_telemetryData_01.m_GPSLocked) &&
		(m_telemetryData_01.m_currentTime != TELEMETRY_BAD_FLOAT))
	{
		labelText = formatDoubleTime(m_telemetryData_01.m_currentTime);
		m_ctrlCurrentTime->SetLabel(labelText);
	}
	else
		m_ctrlCurrentTime->SetLabel(_("ERROR"));

	// Door Status
	if(m_telemetryData_01.m_doorStatus == doorState_unknown)
	{
		m_ctrlDoorStatus->SetLabel(_("Unknown"));
		m_ctrlForceDoorButton->SetLabel(_(" -- "));
		m_ctrlForceDoorButton->Enable(false);
	}

	if(m_telemetryData_01.m_doorStatus == doorState_closed)
	{
		m_ctrlDoorStatus->SetLabel(_("Closed"));
		m_ctrlForceDoorButton->SetLabel(_("Open"));
		m_ctrlForceDoorButton->Enable(true);
	}

	if(m_telemetryData_01.m_doorStatus == doorState_open)
	{
		m_ctrlDoorStatus->SetLabel(_("Open"));
		m_ctrlForceDoorButton->SetLabel(_("Close"));
		m_ctrlForceDoorButton->Enable(true);
	}


	if(m_telemetryData_01.m_doorStatus == doorState_moving)
	{
		m_ctrlDoorStatus->SetLabel(_("Moving"));
		m_ctrlForceDoorButton->SetLabel(_(" -- "));
		m_ctrlForceDoorButton->Enable(false);
	}

	// Door times
	labelText = _("Unknown");
	if((m_telemetryData_01.m_doorOpenTime != TELEMETRY_BAD_FLOAT) &&
			(m_telemetryData_01.m_doorCloseTime != TELEMETRY_BAD_FLOAT))
	{
		labelText = labelText.Format("%s - %s",
									 formatDoubleTime(m_telemetryData_01.m_doorOpenTime),
									 formatDoubleTime(m_telemetryData_01.m_doorCloseTime));
	}
	m_ctrlDoorTimes->SetLabel(labelText);

	// Light times
	labelText = _("Unknown");
	if((m_telemetryData_01.m_morningLightOnTime != TELEMETRY_BAD_FLOAT) &&
			(m_telemetryData_01.m_morningLightOffTime != TELEMETRY_BAD_FLOAT))
	{
		if(EFFECTIVELY_EQUAL(m_telemetryData_01.m_morningLightOnTime,
							 m_telemetryData_01.m_morningLightOffTime))
		{
			labelText = _(" -- ");
		}
		else
		{
			labelText = labelText.Format("%s - %s",
										 formatDoubleTime(m_telemetryData_01.m_morningLightOnTime),
										 formatDoubleTime(m_telemetryData_01.m_morningLightOffTime));
		}
	}
	m_ctrlMorningLightTimes->SetLabel(labelText);

	labelText = _("Unknown");
	if((m_telemetryData_01.m_eveningLightOnTime != TELEMETRY_BAD_FLOAT) &&
			(m_telemetryData_01.m_eveningLightOffTime != TELEMETRY_BAD_FLOAT))
	{
		if(EFFECTIVELY_EQUAL(m_telemetryData_01.m_eveningLightOnTime,
							 m_telemetryData_01.m_eveningLightOffTime))
		{
			labelText = _(" -- ");
		}
		else
		{
			labelText = labelText.Format("%s - %s",
										 formatDoubleTime(m_telemetryData_01.m_eveningLightOnTime),
										 formatDoubleTime(m_telemetryData_01.m_eveningLightOffTime));
		}
	}
	m_ctrlEveningLightTimes->SetLabel(labelText);

	// Light Status
	if(m_telemetryData_01.m_lightIsOn)
	{
		m_ctrlLightStatus->SetLabel(_("On"));
		m_ctrlForceLightButton->SetLabel(_("Off"));
		m_ctrlForceLightButton->Enable(true);
	}
	else
	{
		m_ctrlLightStatus->SetLabel(_("Off"));
		m_ctrlForceLightButton->SetLabel(_("On"));
		m_ctrlForceLightButton->Enable(true);
	}

	if(!m_commsAreUp)
	{
		m_ctrlForceDoorButton->Enable(false);
		m_ctrlForceLightButton->Enable(false);
	}

	Refresh();
	sendTelemetryVersion(false);

	// Screen for startup delay so that there is not
	// a bunch of error notification while we wait for
	// telemetry to start streaming in.
	if(time(0L) < initialNotificationDelay)
		return;

	// Check comms status
	if(m_commsAreUp)
		g_notifyCoopCommStatus.status(CChangeNotifier_status_restore);
	else
		g_notifyCoopCommStatus.status(CChangeNotifier_status_loss, CChangeNotifier_force_periodic);

	// These only make sense if the comm port is open
	if(m_commsAreUp)
	{
		// GPS lock status
		if(m_telemetryData_01.m_GPSLocked)
			g_notifyCoopGPSLockStatus.status(CChangeNotifier_status_restore);
		else
			g_notifyCoopGPSLockStatus.status(CChangeNotifier_status_loss, CChangeNotifier_force_periodic);

		// Door status
		if(m_telemetryData_01.m_doorStatus == doorState_closed)
			g_notifyCoopDoorStatus.status(CChangeNotifier_status_loss);

		if(m_telemetryData_01.m_doorStatus == doorState_open)
			g_notifyCoopDoorStatus.status(CChangeNotifier_status_restore);

		// Light status
		if(m_telemetryData_01.m_lightIsOn)
			g_notifyCoopLightStatus.status(CChangeNotifier_status_restore);
		else
			g_notifyCoopLightStatus.status(CChangeNotifier_status_loss);

		// Check for errors and notify if needed. This is tricky because
		// errors can appear, but there is nothing to let us know when they
		// go away
		// GPS Problems (other than not being locked)
		if ((m_telemetryData_01.m_errorCode & telemetry_error_GPS_no_data) ||
				(m_telemetryData_01.m_errorCode & telemetry_error_GPS_bad_data))
			g_notifyCoopGPSError.status(CChangeNotifier_status_loss, CChangeNotifier_force_periodic);
		else
			g_notifyCoopGPSError.status(CChangeNotifier_status_restore, CChangeNotifier_force_none);

		// Specific door errors
		if ((m_telemetryData_01.m_errorCode & telemetry_error_no_door_motor) ||
				(m_telemetryData_01.m_errorCode & telemetry_error_door_motor_unknown_state) ||
				m_telemetryData_01.m_errorCode & telemetry_error_door_motor_unknown_not_responding)
			g_notifyCoopDoorError.status(CChangeNotifier_status_loss, CChangeNotifier_force_periodic);
		else
			g_notifyCoopDoorError.status(CChangeNotifier_status_restore, CChangeNotifier_force_none);

		// General controller errors
		if (m_telemetryData_01.m_errorCode & telemetry_error_suncalc_invalid_time)
			g_notifyCoopControllerError.status(CChangeNotifier_status_loss, CChangeNotifier_force_periodic);
		else
			g_notifyCoopControllerError.status(CChangeNotifier_status_restore, CChangeNotifier_force_none);
	}
}

void VeronicaDialog::OnClose(wxCloseEvent & event)
{
	Destroy();
}

void VeronicaDialog::OnQuit(wxCommandEvent & event)
{
	Destroy();
}

void VeronicaDialog::onTimer_updateTimer(wxTimerEvent & WXUNUSED(_e))
{
	UpdateControls();

	// Command sender
	m_commandSender->tick();
}

void VeronicaDialog::onTimer_pollTimer(wxTimerEvent & WXUNUSED(_e))
{
	// If the serial port is open then poll it
	if(m_serialComm.isOpen())
	{
		m_serialComm.tick();

		if(m_serialComm.bytesInReceiveBuffer() > 0)
		{
			// If the comms just came up, send the telemetry
			// version to get things going
			if(!m_commsAreUp)
				sendTelemetryVersion(true);

			m_lastCommData = time(0);
			m_commsAreUp = true;
		}
		m_telemetry.tick();
	}
	else
	{
		// Try opening it and report an error
		// if it does not open
		m_serialComm.open(m_serialPortName.c_str(), m_serialBaudRate, 8, CSerialComm_Parity_None, 1);
		if(!m_serialComm.isOpen() && m_reportDeadSerial)
		{
			wxMessageBox(_("The serial port did not open.\n"
						   "Please go to 'Settings' and choose another port"),
						 _("Serial port error"),
						 wxOK | wxCENTER | wxICON_ERROR,
						 this);
			m_reportDeadSerial = false;
		}
	}

	// Check for comms down
	if((m_lastCommData + COMM_DATA_TIMEOUT) < time(0))
	{
		m_commsAreUp = false;
		m_telemetryData_01.clearCommsRelated();
		m_telemetryData_01.clearGPSRelated();
	}

	// If the GPS is not locked then all data is suspect
	if(!m_telemetryData_01.m_GPSLocked)
		m_telemetryData_01.clearGPSRelated();

	// Restart the poll timer
	m_pollTimer.Start(POLL_TIMER_MS, true);
}

void VeronicaDialog::sendTelemetryVersion(bool _force)
{
	if((_force) || (time(0L) > m_timeToSendTelemetryVersion))
	{
		m_timeToSendTelemetryVersion = time(0L) + SEND_PROTOCOL_VERSION_TIME;
		m_commandSender->sendCommand(telemetry_command_version, TELEMETRY_VERSION_01);
	}
}

void VeronicaDialog::showNakDialog(int _tag, double _value, int _reason)
{
	int command = _tag;
	int reason = _reason;

#ifdef __UNIX_LIKE__
	wxString eol("\n");
#endif

#ifdef __WXMSW__
	wxString eol("\r\n");
#endif

	wxString commandS;
	switch(command)
	{
	case telemetry_command_version:
		commandS = _("Set telemetry version");
		break;

	case telemetry_command_setSunriseOffset:
		commandS = _("Set sunrise offset");
		break;

	case telemetry_command_setSunsetOffset:
		commandS = _("Set sunset offset");
		break;

	case telemetry_command_setMinimumDayLength:
		commandS = _("Set minimum day length");
		break;

	case telemetry_command_setExtraIlluminationMorning:
		commandS = _("Set morning extra light time");
		break;

	case telemetry_command_setExtraIlluminationEvening:
		commandS = _("Set evening extra light time");
		break;

	case telemetry_command_forceDoor:
		commandS = _("Force door");
		break;

	case telemetry_command_forceLight:
		commandS = _("Force light");
		break;

	case telemetry_command_setStuckDoorDelay:
		commandS = _("Set stuck door delay");
		break;

	case telemetry_command_loadDefaults:
		commandS = _("Load defaults");
		break;

	default:
		commandS = _("Unknown command Nak'd");
		break;
	}

	wxString reasonS;
	switch(reason)
	{
	case telemetry_cmd_response_nak_version_not_set:
		reasonS = _("Protocol version not yet set.");
		break;

	case telemetry_cmd_response_nak_invalid_command:
		reasonS = _("Invalid (unknown) command.");
		break;

	case telemetry_cmd_response_nak_invalid_value:
		reasonS = _("Invalid value (Range).");
		break;

	case telemetry_cmd_response_nak_not_ready:
		reasonS = _("Object net ready (bad state).");
		break;

	case telemetry_cmd_response_nak_internal_error:
		reasonS = _("Internal error... Consult the code.");
		break;

	default:
		reasonS = _("Unknown Nak reason");
		break;
	}

	wxString valueS = wxString::Format("Value: %.02f", _value);

	// Construct the message
	wxString messageS = commandS;
	messageS += eol;
	messageS += reasonS;
	messageS += eol;
	messageS += valueS;

	wxMessageBox(messageS, _("Error: Command Nak'd"),
				 wxOK | wxCENTER | wxICON_ERROR);
}

void VeronicaDialog::OnButton_ForceDoor(wxCommandEvent & WXUNUSED(_e))
{
	if((m_telemetryVersion != TELEMETRY_VERSION_01) ||
			(!m_commsAreUp))
	{
		wxMessageBox(_("Comm or telemetry problem...\nno commands may be sent."),
					_("Problem"),
					wxOK | wxCENTER | wxICON_EXCLAMATION,
					this);
		return;
	}

	int doorStatus = m_telemetryData_01.m_doorStatus;
	if(doorStatus != doorState_unknown)
	{
		if(wxMessageBox(_("Are you sure?"),
						_("Verification"),
						wxYES_NO | wxCANCEL | wxNO_DEFAULT,
						this) == wxNO)
		{
			return;
		}
	}

	m_commandSender->sendCommand(telemetry_command_forceDoor,
								(doorStatus != doorCommand_open) ? doorCommand_open : doorCommand_close);
}

void VeronicaDialog::OnButton_ForceLight(wxCommandEvent & WXUNUSED(_e))
{
	if((m_telemetryVersion != TELEMETRY_VERSION_01) ||
			(!m_commsAreUp))
	{
		wxMessageBox(_("Comm or telemetry problem...\nno commands may be sent."),
					_("Problem"),
					wxOK | wxCENTER | wxICON_EXCLAMATION,
					this);
		return;
	}

	if(wxMessageBox(_("Are you sure?"),
					_("Verification"),
					wxYES_NO | wxCANCEL | wxNO_DEFAULT,
					this) == wxNO)
	{
		return;
	}


	m_commandSender->sendCommand(telemetry_command_forceLight,
								(m_telemetryData_01.m_lightIsOn ? 0 : 1));

}

void VeronicaDialog::OnButton_OpenSettings(wxCommandEvent & WXUNUSED(_e))
{
	CSettingsDialog settingsDialog(this, _("Veronica Settings"),
								   m_telemetryData_01);

	// If we select cancel then just destroy it and go
	if (settingsDialog.ShowModal() == wxID_CANCEL)
	{
		settingsDialog.Destroy();
		return;     // the user changed his mind
	}

	int sunriseOffset = m_telemetryData_01.m_sunriseOffset;
	int sunsetOffset = m_telemetryData_01.m_sunsetOffset;

	double minDayLength = m_telemetryData_01.m_minimumDayLength;

	double extraLightMorning = m_telemetryData_01.m_extraLightTimeMorning;
	double extraLightEvening = m_telemetryData_01.m_extraLightTimeEvening;

	// Send the settings established by the settings dialog.
	// If GPS was not locked at the time, then do not send
	// things that depend on the time being correct. That means
	// pretty much any setting in the controller.
	if(settingsDialog.gpsWasLocked())
	{
		if(sunriseOffset != settingsDialog.getSunriseOffset())
		{
			m_commandSender->sendCommand(telemetry_command_setSunriseOffset,
										 settingsDialog.getSunriseOffset());
		}

		if(sunsetOffset != settingsDialog.getSunsetOffset())
		{
			m_commandSender->sendCommand(telemetry_command_setSunsetOffset,
										 settingsDialog.getSunsetOffset());
		}

		if(minDayLength != settingsDialog.getDayLength())
		{
			m_commandSender->sendCommand(telemetry_command_setMinimumDayLength,
										 settingsDialog.getDayLength());
		}

		if(extraLightMorning != settingsDialog.getExtraLightMorning())
		{
			m_commandSender->sendCommand(telemetry_command_setExtraIlluminationMorning,
										 settingsDialog.getExtraLightMorning());
		}

		if(extraLightEvening != settingsDialog.getExtraLightEvening())
		{
			m_commandSender->sendCommand(telemetry_command_setExtraIlluminationEvening,
										 settingsDialog.getExtraLightEvening());
		}
	}

	// So, load our settings
	LoadSettings();

	// The user may have selected another serial port,
	// so we might report it again!
	m_reportDeadSerial = true;

	// All done with the dialog
	settingsDialog.Destroy();
}

void VeronicaDialog::LoadSettings()
{
	// Save them before anything could go wrong
	wxConfig::Get()->Flush();

	// Serial port info
	wxConfig::Get()->Read(config_key_serial_port,
						  &m_serialPortName,
						  wxString("/dev/ttyUSB0"));

	wxString baudRate = wxString::Format("%d", TELEMETRY_BAUD_RATE);
	wxConfig::Get()->Read(config_key_serial_baud,
						  &baudRate,
						  baudRate);
	m_serialBaudRate = atoi(baudRate.c_str());

	// Close the serial port. It will be re-opened
	// in the timer function
	m_serialComm.close();

	// Get the mute settings for the various voice alerts
	bool muteLoss = false;
	bool muteRestore = false;
	bool muteBoth = false;

	// Door
	wxConfig::Get()->Read(config_key_voice_alert_door_close, &muteLoss, false);
	wxConfig::Get()->Read(config_key_voice_alert_door_open, &muteRestore, false);
	g_notifyCoopDoorStatus.mute(!muteLoss, !muteRestore);

	// Light
	wxConfig::Get()->Read(config_key_voice_alert_light_off, &muteLoss, false);
	wxConfig::Get()->Read(config_key_voice_alert_light_on, &muteRestore, false);
	g_notifyCoopLightStatus.mute(!muteLoss, !muteRestore);

	// Comms
	wxConfig::Get()->Read(config_key_voice_warning_comm, &muteBoth, false);
	g_notifyCoopCommStatus.mute(!muteBoth, !muteBoth);

	// GPS
	wxConfig::Get()->Read(config_key_voice_warning_gps, &muteBoth, false);
	g_notifyCoopGPSLockStatus.mute(!muteBoth, !muteBoth);
}

//////////////////////////////////////////////////////////
// Receive telemetry


////////////////////////////////////////
// Parser target commands
void VeronicaDialog::startReception()
{
	// Clear everything
	m_telemData.Clear();
}

void VeronicaDialog::receiveTerm(int _index, const char *_value)
{
	// If there is no data then bolt
	if(!_value)
		return;

	// Stash it until I know the checksum is correct
	m_telemData.Add(wxString(_value));
}

void VeronicaDialog::receiveChecksumCorrect()
{
	// The first term is the tag
	if(m_telemData.GetCount() < 1)
	{
		m_telemData.Clear();
		return;
	}

	int tag = atoi(m_telemData[0].c_str());

	// Special version screen
	if(m_telemetryVersion == TELEMETRY_VERSION_INVALID)
	{
		if(tag == telemetry_tag_version)
		{
			// We have a version!
			int version = atoi(m_telemData[1].c_str());
			if(version == TELEMETRY_VERSION_01)
				m_telemetryVersion = TELEMETRY_VERSION_01;
		}

		m_telemData.Clear();
		return;
	}

	if(m_telemetryVersion == TELEMETRY_VERSION_01)
	{
		m_telemetryData_01.processTelemetry(m_telemData);

		// See if we got an ack. If so then remove it from the queue
		if(m_telemetryData_01.m_commandAckTag != TELEMETRY_BAD_INT)
		{
			m_commandSender->gotAck(m_telemetryData_01.m_commandAckTag,
									m_telemetryData_01.m_commandAckValue);

			m_telemetryData_01.m_commandAckTag = TELEMETRY_BAD_INT;
			m_telemetryData_01.m_commandAckValue = TELEMETRY_BAD_FLOAT;
		}

		// See if we got a nak. If so then remove it from the queue
		if(m_telemetryData_01.m_commandNakTag != TELEMETRY_BAD_INT)
		{
			showNakDialog(m_telemetryData_01.m_commandNakTag,
						  m_telemetryData_01.m_commandNakValue,
						  m_telemetryData_01.m_commandNakReason);

			m_commandSender->gotNak(m_telemetryData_01.m_commandNakTag,
									m_telemetryData_01.m_commandNakValue);

			m_telemetryData_01.m_commandNakTag = TELEMETRY_BAD_INT;
			m_telemetryData_01.m_commandNakValue = TELEMETRY_BAD_FLOAT;
			m_telemetryData_01.m_commandNakReason = TELEMETRY_BAD_INT;
		}
	}

	// Make sure to clear the telem data just to be sure.
	m_telemData.Clear();
}

void VeronicaDialog::receiveChecksumError()
{
	/*
	#ifdef DEBUG
		// Darn
		wxString msg;

		for(int _ = 0; _ < NDATA; ++_)
		{
			if(g_telemData[_].length() == 0)
				break;
			if(_ > 0)
				msg += wxString(",");
			msg += g_telemData[_];
		}

		wxMessageBox(msg, _("Bad checksum for telemetry"));
	#endif
	*/

	m_telemData.Clear();
}

wxString VeronicaDialog::formatDoubleTime(double _t)
{
	// Verify that it makes sense
	if((_t < 0.) || (_t >= 24.))
	{
		return wxString(_("Bad Time"));
	}

	// Start with what was provided
	int hour = (int)_t;
	int minute = 60. * (_t - hour);

	wxDateTime normalizedTime(m_telemetryData_01.m_day,
							  wxDateTime::Month(m_telemetryData_01.m_month - 1),
							  m_telemetryData_01.m_year,
							  hour, minute);
	if(!normalizedTime.IsValid())
		return wxString(_("Bad Time"));

#ifndef SHOW_UTC
	normalizedTime.MakeFromTimezone(wxDateTime::UTC);
#endif

	hour = normalizedTime.GetHour();
	minute = normalizedTime.GetMinute();

	// Format it
	return wxString::Format(_("%02d:%02d"), hour, minute);
}
