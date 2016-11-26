////////////////////////////////////////////////////////////
// Serial Port Dialog
////////////////////////////////////////////////////////////
#ifndef CSERIALPORTDIALOG_H
#define CSERIALPORTDIALOG_H

////////////////////////////////////////////////////////////
// This class will create a dialog for selecting a serial
// port and baud rate
////////////////////////////////////////////////////////////
class CSerialPortDialog : public wxDialog
{
	protected:
		wxString m_sSelectedPort;
#ifdef __WXMSW__
		wxChoice *m_pPortBoxCtrl;
#endif

#ifdef __UNIX_LIKE__
		wxTextCtrl *m_pDevicePathTextCtrl;
#endif
		wxChoice *m_pBaudRateBoxCtrl;

		long m_iBaudRate;

	private:
		void OnOK(wxCommandEvent& event);
		void OnCancel(wxCommandEvent& event);
		void OnClose(wxCloseEvent& event);

#ifdef __UNIX_LIKE__
		void OnDevicePathTextCtrlChange(wxCommandEvent& _e);
		void OnDevicePathTextCtrlEnter(wxCommandEvent& _e);
		void OnDevicePathBrowse(wxCommandEvent& _e);
#endif
		void OnBaudChange(wxCommandEvent& _e);

#ifdef __WXMSW__
		void OnPortChange(wxCommandEvent& _e);
#endif
		DECLARE_EVENT_TABLE()
public:
	CSerialPortDialog(wxWindow *_parent, const wxString& _sTitle, wxString &_sDefaultPort, int _iDefaultBaud);
	virtual ~CSerialPortDialog();

	wxString GetPort() { return m_sSelectedPort; }
	int GetBaud() { return (int)m_iBaudRate; }
};

#endif
