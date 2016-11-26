////////////////////////////////////////////////////////////
// Serial Port Dialog
////////////////////////////////////////////////////////////
#ifndef SettingsDialog_H
#define SettingsDialog_H

////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
class CSettingsDialog : public wxDialog
{
protected:
	enum
	{
		idSlider_sunriseOffset,
		idSlider_sunsetOffset,

		idSlider_dayLength,
		idSlider_extraLight,

		idButton_selectPort,

		idCheck_doorVoice_Open,
		idCheck_doorVoice_Close,

		idCheck_lightVoiceOn,
		idCheck_lightVoiceOff,

		idCheck_commsWarning,
		idCheck_GPSWarning,
	};

	wxSlider *m_sliderSunriseOffset;
	wxSlider *m_sliderSunsetOffset;

	wxStaticText *m_textDoorOpen;
	wxStaticText *m_textDoorClose;

	wxSlider *m_sliderDayLength;
	wxSlider *m_sliderExtraLight;

	wxStaticText *m_textDayLength;
	wxStaticText *m_textExtraLight;

	wxCheckBox *m_checkDoorVoiceOpen;
	wxCheckBox *m_checkDoorVoiceClose;

	wxCheckBox *m_checkLightVoiceOn;
	wxCheckBox *m_checkLightVoiceOff;

	wxCheckBox *m_checkCommsWarning;
	wxCheckBox *m_checkGPSWarning;

	CTelemetryData_01 *m_telemData;
	bool m_gpsWasLocked;

	// Actual data from the settings box
	int m_sunriseOffset;
	int m_sunsetOffset;

	double m_dayLength;
	double m_extraLight;

	wxString m_serialPortName;
	int m_baudRate;

private:
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	void OnSliderChange(wxScrollEvent& _e);
	void OnButton_SelectPort(wxCommandEvent &_e);

	void UpdateTextControls();

	DECLARE_EVENT_TABLE()
public:
	CSettingsDialog(wxWindow *_parent, const wxString& _title,
					CTelemetryData_01 &_telemData);
	virtual ~CSettingsDialog();

	void addDoubleTimeToArrayString(CTelemetryData_01 &_telem,
					   double _time,
					   wxArrayString &_arrayString);

	wxString formatDoubleTime(double _t);

	int getSunriseOffset() { return m_sunriseOffset; }
	int getSunsetOffset() { return m_sunsetOffset; }

	double getDayLength() { return m_dayLength; }
	double getExtraLight() { return m_extraLight; }

	bool gpsWasLocked() { return m_gpsWasLocked; }
};

#endif
