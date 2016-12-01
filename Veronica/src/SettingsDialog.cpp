////////////////////////////////////////////////////////////
// Serial Port Dialog
////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/config.h>

#include "../../GaryCooper/TelemetryTags.h"

#include "TelemetryData01.h"
#include "SettingsDialog.h"
#include "ConfigKeys.h"
#include "SerialPortDialog.h"
#include "TimeDisplayFormat.h"

#define BOXBORDER (5)
#define CONTROLBORDER (3)
#define SLIDER_WIDTH (240)

#define SERIAL_DEFAULT_BAUD (115200)

// With the day-length slider, as with all sliders,
// we're working with integer ticks, not actual time values.
// I need ticks to get me from 0 to 16 hours on fifteen minute increments, with
// an initial value of 12. Since there are 4 fifteen minute periods in an hour,
// I have multiplied everything by four. The macro names are "To Day Length Slider Ticks"
// and "From Day Length Slider Ticks"
#define ToDLSTicks(hrs)	((int)(hrs * 4.))
#define FromDLSTicks(ticks)	((double)(ticks / 4.))

#define ToELSTicks(hrs)	((int) roundf(hrs * 60.))
#define FromELSTicks(ticks) ((double)(ticks / 60.))

////////////////////////////////////////////////////////////
void normalizeTime(double &_t)
{
	while (_t < 0.) _t += 24.;
	while (_t > 24.) _t -= 24.;
}

void readCheckBoxSetting(wxCheckBox *_box, wxString &_configKey, bool _def = true)
{
	if(!_box) return;
	bool v = false;
	wxConfig::Get()->Read(_configKey, &v, _def);
	_box->SetValue(v);
}

void writeCheckBoxSetting(wxCheckBox *_box, wxString &_configKey)
{
	if(!_box) return;
	wxConfig::Get()->Write(_configKey, _box->IsChecked());
}

////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE(CSettingsDialog, wxDialog)
	EVT_SCROLL(CSettingsDialog::OnSliderChange)
	EVT_BUTTON(idButton_selectPort, CSettingsDialog::OnButton_SelectPort)

	EVT_BUTTON(wxID_OK, CSettingsDialog::OnOK)
	EVT_BUTTON(wxID_CANCEL, CSettingsDialog::OnCancel)
	EVT_CLOSE(CSettingsDialog::OnClose)
END_EVENT_TABLE()


CSettingsDialog::CSettingsDialog(wxWindow *_parent, const wxString &title,
								 CTelemetryData_01 &_telemData)
	: wxDialog(_parent, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	// Remember the telemetry data
	m_telemData = &_telemData;
	m_gpsWasLocked = m_telemData->m_GPSLocked;

	// Gather needed data
	m_sunriseOffset = m_telemData->m_sunriseOffset;
	m_sunsetOffset = m_telemData->m_sunsetOffset;

	/////////////////////////////////////
	// Create control contents
	wxPanel *panel = new wxPanel(this);
	wxStaticText *label;

	/////////////////////////////////////
	// Now create the top-level sizer.  It is a box
	// that stacks the other sizers vertically.
	// It contains two sizers, one for the controls and another for
	// the buttons
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	/////////////////////////////////////////////////////////////////////
	// Create a sizer for the sunrise and sunset selections
	{
		wxStaticBoxSizer *doorScheduleSizer = new wxStaticBoxSizer(
			wxVERTICAL,
			panel,
			_("Door Schedule"));
		mainSizer->Add(doorScheduleSizer,
					   wxSizerFlags().
					   Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
					   Expand());

		// Door open
		m_textDoorOpen = new wxStaticText(panel, -1, _("Sunrise XX:XX +Offset = Door Open Time XX:XX"),
										  wxDefaultPosition, wxDefaultSize,
										  wxFIXED_MINSIZE);
		doorScheduleSizer->Add(m_textDoorOpen,
							   wxSizerFlags().
							   Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
							   Center());

		m_sliderSunriseOffset = new wxSlider(panel, idSlider_sunriseOffset,
											 m_sunriseOffset, -GARY_COOPER_DOOR_MAX_TIME_OFFSET, GARY_COOPER_DOOR_MAX_TIME_OFFSET,
											 wxDefaultPosition, wxSize(SLIDER_WIDTH, 25),
											 wxSL_HORIZONTAL);
		m_sliderSunriseOffset->SetTickFreq(1);
		doorScheduleSizer->Add(m_sliderSunriseOffset,
							   wxSizerFlags().
							   Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
							   Expand());

		// Door close
		m_textDoorClose = new wxStaticText(panel, -1, _("Sunset XX:XX +Offset = Door Open Time XX:XX"),
										   wxDefaultPosition, wxDefaultSize,
										   wxFIXED_MINSIZE);
		doorScheduleSizer->Add(m_textDoorClose,
							   wxSizerFlags().
							   Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
							   Center());

		m_sliderSunsetOffset = new wxSlider(panel, idSlider_sunsetOffset,
											m_sunsetOffset, -GARY_COOPER_DOOR_MAX_TIME_OFFSET, GARY_COOPER_DOOR_MAX_TIME_OFFSET,
											wxDefaultPosition, wxSize(SLIDER_WIDTH, 25),
											wxSL_HORIZONTAL);
		m_sliderSunsetOffset->SetTickFreq(1);
		doorScheduleSizer->Add(m_sliderSunsetOffset,
							   wxSizerFlags().
							   Border(wxALL, BOXBORDER).
							   Expand());
	}

	/////////////////////////////////////////////////////////////////////
	// Create a sizer for the Illumination duration controls
	{
		wxStaticBoxSizer *lightSizer = new wxStaticBoxSizer(
			wxVERTICAL,
			panel,
			_("Light Schedule"));
		mainSizer->Add(lightSizer,
					   wxSizerFlags().
					   Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
					   Expand());

		wxFlexGridSizer *flexGrid = new wxFlexGridSizer(3, BOXBORDER, BOXBORDER);
		lightSizer->Add(flexGrid);

		// Day length
		label = new wxStaticText(panel, -1, _("Day Length (0 - 16 hrs)"),
								 wxDefaultPosition, wxDefaultSize,
								 wxFIXED_MINSIZE);
		flexGrid->Add(label,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP, BOXBORDER));

		int dayLengthValueTicks = 0;
		if(m_telemData->m_minimumDayLength != TELEMETRY_BAD_FLOAT)
			dayLengthValueTicks = ToDLSTicks(m_telemData->m_minimumDayLength);
		else
			dayLengthValueTicks = 0;

		m_sliderDayLength = new wxSlider(panel, idSlider_dayLength,
										 dayLengthValueTicks, ToDLSTicks(0), ToDLSTicks(GARY_COOPER_LIGHT_MAX_DAY_LENGTH),
										 wxDefaultPosition, wxSize(SLIDER_WIDTH, 25),
										 wxSL_HORIZONTAL);
		m_sliderDayLength->SetTickFreq(1);
		m_dayLength = FromDLSTicks(m_sliderDayLength->GetValue());

		flexGrid->Add(m_sliderDayLength,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
					  Expand());

		m_textDayLength = new wxStaticText(panel, -1, _("    "),
										   wxDefaultPosition, wxDefaultSize,
										   wxFIXED_MINSIZE);
		flexGrid->Add(m_textDayLength,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER));

		// Morning Extra light
		label = new wxStaticText(panel, -1, _("Extra Morning Light (0 - 60 min)"),
								 wxDefaultPosition, wxDefaultSize,
								 wxFIXED_MINSIZE);
		flexGrid->Add(label,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP, BOXBORDER));

		int elValMorning = ToELSTicks(m_telemData->m_extraLightTimeMorning);
		int elMin = ToELSTicks(GARY_COOPER_LIGHT_MIN_EXTRA);
		int elMax = ToELSTicks(GARY_COOPER_LIGHT_MAX_EXTRA);

		m_sliderExtraLightMorning = new wxSlider(panel, idSlider_extraLightMorning,
										  elValMorning, elMin, elMax,
										  wxDefaultPosition, wxSize(SLIDER_WIDTH, 25),
										  wxSL_HORIZONTAL);
		m_sliderExtraLightMorning->SetTickFreq(1);
		m_extraLightMorning = (double)((double)m_sliderExtraLightMorning->GetValue() / 60.);

		flexGrid->Add(m_sliderExtraLightMorning,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
					  Expand());

		m_textExtraLightMorning = new wxStaticText(panel, -1, _("    "),
											wxDefaultPosition, wxDefaultSize,
											wxFIXED_MINSIZE);
		flexGrid->Add(m_textExtraLightMorning,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER));

		// Evening Extra light
		label = new wxStaticText(panel, -1, _("Extra Evening Light (0 - 60 min)"),
								 wxDefaultPosition, wxDefaultSize,
								 wxFIXED_MINSIZE);
		flexGrid->Add(label,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP, BOXBORDER));

		int elValEvening = ToELSTicks(m_telemData->m_extraLightTimeEvening);

		m_sliderExtraLightEvening = new wxSlider(panel, idSlider_extraLightEvening,
										  elValEvening, elMin, elMax,
										  wxDefaultPosition, wxSize(SLIDER_WIDTH, 25),
										  wxSL_HORIZONTAL);
		m_sliderExtraLightEvening->SetTickFreq(1);
		m_extraLightEvening = (double)((double)m_sliderExtraLightEvening->GetValue() / 60.);

		flexGrid->Add(m_sliderExtraLightEvening,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER).
					  Expand());

		m_textExtraLightEvening = new wxStaticText(panel, -1, _("    "),
											wxDefaultPosition, wxDefaultSize,
											wxFIXED_MINSIZE);
		flexGrid->Add(m_textExtraLightEvening,
					  wxSizerFlags().
					  Border(wxLEFT | wxTOP | wxRIGHT, BOXBORDER));
	}

	/////////////////////////////////////////////////////////////////////
	// A sizer to put the voice alerts and serial port buttons
	// beside each other
	{
		wxBoxSizer *voice_serial_sizer = new wxBoxSizer(wxHORIZONTAL);
		mainSizer->Add(voice_serial_sizer);

		/////////////////////////////////////////////////////////////////////
		// Create a sizer for the voice alert checkboxes
		{
			wxStaticBoxSizer *voiceAlertSizer = new wxStaticBoxSizer(
				wxVERTICAL,
				panel,
				_("Voice Alerts & Startup"));
			voice_serial_sizer->Add(voiceAlertSizer,
									wxSizerFlags().
									Border(wxALL, BOXBORDER));

			{
				wxFlexGridSizer *flexGrid = new wxFlexGridSizer(2, BOXBORDER, BOXBORDER);
				voiceAlertSizer->Add(flexGrid);

				// Now the check boxes
				m_checkDoorVoiceOpen = new wxCheckBox(panel, idCheck_doorVoice_Open,
													  _("Door Open"));
				flexGrid->Add(m_checkDoorVoiceOpen,
							  wxSizerFlags().
							  Border(wxLEFT | wxRIGHT | wxTOP, CONTROLBORDER));

				m_checkDoorVoiceClose = new wxCheckBox(panel, idCheck_doorVoice_Close,
													   _("Door Close"));
				flexGrid->Add(m_checkDoorVoiceClose,
							  wxSizerFlags().
							  Border(wxLEFT | wxRIGHT | wxTOP, CONTROLBORDER));

				m_checkLightVoiceOn = new wxCheckBox(panel, idCheck_lightVoiceOn,
													 _("Light On"));
				flexGrid->Add(m_checkLightVoiceOn,
							  wxSizerFlags().
							  Border(wxLEFT | wxRIGHT | wxTOP, CONTROLBORDER));

				m_checkLightVoiceOff = new wxCheckBox(panel, idCheck_lightVoiceOff,
													  _("Light Off"));
				flexGrid->Add(m_checkLightVoiceOff,
							  wxSizerFlags().
							  Border(wxLEFT | wxRIGHT | wxTOP, CONTROLBORDER));

				m_checkCommsWarning = new wxCheckBox(panel, idCheck_commsWarning,
													 _("Comm Warnings"));
				flexGrid->Add(m_checkCommsWarning,
							  wxSizerFlags().
							  Border(wxLEFT | wxRIGHT | wxTOP, CONTROLBORDER));

				m_checkGPSWarning = new wxCheckBox(panel, idCheck_GPSWarning,
												   _("GPS Warnings"));
				flexGrid->Add(m_checkGPSWarning,
							  wxSizerFlags().
							  Border(wxLEFT | wxRIGHT | wxTOP, CONTROLBORDER));

				m_checkStartMinimized = new wxCheckBox(panel, idCheck_startMinimized,
												   _("Start Minimized"));
				flexGrid->Add(m_checkStartMinimized,
							  wxSizerFlags().
							  Border(wxLEFT | wxRIGHT | wxTOP, CONTROLBORDER));


			}
		}

		/////////////////////////////////////////////////////////////////////
		// Create a sizer for the Serial port settings button
		{
			wxStaticBoxSizer *commPorttSizer = new wxStaticBoxSizer(
				wxHORIZONTAL,
				panel,
				_("Communications Port"));
			voice_serial_sizer->Add(commPorttSizer,
									wxSizerFlags().
									Border(wxALL, BOXBORDER));

			wxButton *commPortButton = new wxButton(panel, idButton_selectPort, _("Comm Port Settings..."));
			commPorttSizer->Add(commPortButton,
								wxSizerFlags().
								Border(wxALL, CONTROLBORDER));
		}
	}

	/////////////////////////////////////////////////////////////////////
	// Create a sizer for the ok, cancel, and calibrate buttons
	{
		wxStdDialogButtonSizer *buttonSizer = new wxStdDialogButtonSizer();
		mainSizer->Add(buttonSizer,
					   wxSizerFlags().
					   Border(wxALL, BOXBORDER).
					   Align(wxALIGN_RIGHT));

		wxButton *cancelButton = new wxButton(panel, wxID_CANCEL, _("Cancel"));
		buttonSizer->AddButton(cancelButton);

		wxButton *okButton = new wxButton(panel, wxID_OK, _("OK"));
		buttonSizer->AddButton(okButton);
		buttonSizer->Realize();
	}

	// Set the values stored in the config database
	wxConfig::Get()->Read(config_key_serial_port,
						  &m_serialPortName,
						  wxString("/dev/ttyUSB0"));

	wxString baudRate = wxString::Format("%d", SERIAL_DEFAULT_BAUD);
	wxConfig::Get()->Read(config_key_serial_baud,
						  &baudRate,
						  baudRate);
	m_baudRate = atoi(baudRate.c_str());

	readCheckBoxSetting(m_checkDoorVoiceOpen, config_key_voice_alert_door_open);
	readCheckBoxSetting(m_checkDoorVoiceClose, config_key_voice_alert_door_close);

	readCheckBoxSetting(m_checkLightVoiceOn, config_key_voice_alert_light_on);
	readCheckBoxSetting(m_checkLightVoiceOff, config_key_voice_alert_light_off);

	readCheckBoxSetting(m_checkCommsWarning, config_key_voice_warning_comm);
	readCheckBoxSetting(m_checkGPSWarning, config_key_voice_warning_gps);

	readCheckBoxSetting(m_checkStartMinimized, config_key_start_minimized, false);

	// Disable everything that is not available when GPS is not locked
	if(!m_gpsWasLocked)
	{
		m_sliderSunriseOffset->Enable(false);
		m_sliderSunsetOffset->Enable(false);

		m_sliderDayLength->Enable(false);
		m_sliderExtraLightMorning->Enable(false);
		m_sliderExtraLightEvening->Enable(false);
	}

	// Update the text controls
	UpdateTextControls();

	/////////////////////////////////////
	// Set the layout in motion
	panel->SetAutoLayout(true);
	panel->SetSizer(mainSizer);
	mainSizer->Fit(this);
	SetMinSize(GetSize());
}

CSettingsDialog::~CSettingsDialog()
{

}

void CSettingsDialog::OnSliderChange(wxScrollEvent& _e)
{
	// Which slider is it?

	// Sunrise offset?
	if(_e.GetId() == idSlider_sunriseOffset)
	{
		m_sunriseOffset = _e.GetInt();
	}

	// Sunset offset?
	if(_e.GetId() == idSlider_sunsetOffset)
	{
		m_sunsetOffset = _e.GetInt();
	}

	// Day length in the light schedule?
	if(_e.GetId() == idSlider_dayLength)
	{
		m_dayLength = FromDLSTicks(_e.GetInt());
	}

	// Extra morning light in the light schedule?
	if(_e.GetId() == idSlider_extraLightMorning)
	{
		m_extraLightMorning = FromELSTicks(_e.GetInt());
	}

	// Extra evening light in the light schedule?
	if(_e.GetId() == idSlider_extraLightMorning)
	{
		m_extraLightEvening = FromELSTicks(_e.GetInt());
	}

	// Update other controls
	UpdateTextControls();
}

void CSettingsDialog::OnButton_SelectPort(wxCommandEvent &WXUNUSED(_e))
{
	CSerialPortDialog serialPortDialog(this,
									   _("Select serial port"),
									   m_serialPortName,
									   m_baudRate);

	// If we select cancel then just destroy it and go
	if (serialPortDialog.ShowModal() == wxID_CANCEL)
	{
		serialPortDialog.Destroy();
		return;     // the user changed his mind
	}

	m_baudRate = serialPortDialog.GetBaud();
	m_serialPortName = serialPortDialog.GetPort();

	// All done with the dialog
	serialPortDialog.Destroy();
}

void CSettingsDialog::UpdateTextControls()
{
	int hour = 0;
	int minute = 0;
	wxString textValue;

	wxString offsetTime;
	double doorTime;

	wxString sunriseTime;
	wxString doorOpenTime;
	wxString doorOpenText;

	wxString sunsetTime;
	wxString doorCloseTime;
	wxString doorCloseText;

	// If the GPS is not locked then we cannot change the
	// door schedule
	if(m_telemData->m_GPSLocked)
	{
		///////////////////////////////////////
		// Sunrise schedule
		sunriseTime = formatDoubleTime(m_telemData->m_sunriseTime);

		// Format the offset
		hour = abs(m_sunriseOffset / 60);
		minute = abs(m_sunriseOffset % 60);
		if(m_sunriseOffset >= 0)
			offsetTime = wxString::Format("+ %02d:%02d", hour, minute);
		else
			offsetTime = wxString::Format("- %02d:%02d", hour, minute);

		// Now the door open time
		doorTime = m_telemData->m_sunriseTime + ((double) (m_sunriseOffset / 60.));
		doorOpenTime = formatDoubleTime(doorTime);

		// Bundle it up and set the label
		doorOpenText = wxString::Format("Sunrise %s %s = Door Open Time %s",
										sunriseTime, offsetTime, doorOpenTime);
		m_textDoorOpen->SetLabel(doorOpenText);

		///////////////////////////////////////
		// Sunset schedule
		sunsetTime = formatDoubleTime(m_telemData->m_sunsetTime);

		// Format the offset
		hour = abs(m_sunsetOffset / 60);
		minute = abs(m_sunsetOffset % 60);
		if(m_sunsetOffset >= 0)
			offsetTime = wxString::Format("+ %02d:%02d", hour, minute);
		else
			offsetTime = wxString::Format("- %02d:%02d", hour, minute);

		// Now the door open time
		doorTime = m_telemData->m_sunsetTime + ((double) (m_sunsetOffset / 60.));
		doorCloseTime = formatDoubleTime(doorTime);

		// Bundle it up and set the label
		doorCloseText = wxString::Format("Sunset %s %s = Door Close Time %s",
										sunsetTime, offsetTime, doorCloseTime);
		m_textDoorClose->SetLabel(doorCloseText);
	}
	else
	{
		m_textDoorOpen->SetLabel(_("--"));
		m_textDoorClose->SetLabel(_("--"));
	}

	// Light schedule texts from light schedule sliders
	hour = (int)m_dayLength;
	minute = (int)(60. * (m_dayLength - hour));
	textValue = wxString::Format("%02d:%02d", hour, minute);
	m_textDayLength->SetLabel(textValue);

	m_extraLightMorning = (m_sliderExtraLightMorning->GetValue() / 60.);
	hour = (int)m_extraLightMorning;
	minute = (int)(60. * (m_extraLightMorning - hour));
	textValue = wxString::Format("%02d:%02d", hour, minute);
	m_textExtraLightMorning->SetLabel(textValue);

	m_extraLightEvening = (m_sliderExtraLightEvening->GetValue() / 60.);
	hour = (int)m_extraLightEvening;
	minute = (int)(60. * (m_extraLightEvening - hour));
	textValue = wxString::Format("%02d:%02d", hour, minute);
	m_textExtraLightEvening->SetLabel(textValue);
}

void CSettingsDialog::OnOK(wxCommandEvent &WXUNUSED(_e))
{
	wxConfig::Get()->Write(config_key_serial_port,
						   m_serialPortName);

	wxString baudRate = wxString::Format("%d", m_baudRate);
	wxConfig::Get()->Write(config_key_serial_baud,
						   baudRate);

	writeCheckBoxSetting(m_checkDoorVoiceOpen, config_key_voice_alert_door_open);
	writeCheckBoxSetting(m_checkDoorVoiceClose, config_key_voice_alert_door_close);

	writeCheckBoxSetting(m_checkLightVoiceOn, config_key_voice_alert_light_on);
	writeCheckBoxSetting(m_checkLightVoiceOff, config_key_voice_alert_light_off);

	writeCheckBoxSetting(m_checkCommsWarning, config_key_voice_warning_comm);
	writeCheckBoxSetting(m_checkGPSWarning, config_key_voice_warning_gps);

	writeCheckBoxSetting(m_checkStartMinimized, config_key_start_minimized);

	EndModal(wxID_OK);
}

void CSettingsDialog::OnCancel(wxCommandEvent &WXUNUSED(_e))
{
	EndModal(wxID_CANCEL);
}

void CSettingsDialog::OnClose(wxCloseEvent &WXUNUSED(_e))
{
	EndModal(wxID_CANCEL);
}

wxString CSettingsDialog::formatDoubleTime(double _t)
{
	// Start with what was provided
	int hour = (int)_t;
	int minute = 60. * (_t - hour);

	wxDateTime normalizedTime(m_telemData->m_day,
                           wxDateTime::Month(m_telemData->m_month - 1),
                           m_telemData->m_year,
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
