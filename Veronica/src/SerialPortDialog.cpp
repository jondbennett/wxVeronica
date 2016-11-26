////////////////////////////////////////////////////////////
// Serial Port Dialog
////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/filename.h>

#include "SerialPortDialog.h"
#define BORDERPIXELS (3)

enum
{
	idCtrlBaud = 1000,
	idCtrlDevicePathText,
	idCtrlDevicePathBrowse,
	idCtrlPort      // DOS only
};

////////////////////////////////////////////////////////////
// This class will create a dialog for selecting a serial
// port and baud rate.
// NOTE: This implementation is specific to *NIX and OSs that
// use the file system for devices. Not Windoze
////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE(CSerialPortDialog, wxDialog)
    EVT_BUTTON(wxID_OK, CSerialPortDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, CSerialPortDialog::OnCancel)
    EVT_CLOSE(CSerialPortDialog::OnClose)

#ifdef __UNIX_LIKE__
    EVT_TEXT(idCtrlDevicePathText, CSerialPortDialog::OnDevicePathTextCtrlChange)
	EVT_TEXT_ENTER(idCtrlDevicePathText, CSerialPortDialog::OnDevicePathTextCtrlEnter)
	EVT_BUTTON(idCtrlDevicePathBrowse, CSerialPortDialog::OnDevicePathBrowse)
#endif

	EVT_CHOICE(idCtrlBaud, CSerialPortDialog::OnBaudChange)
#ifdef __WXMSW__
	EVT_CHOICE(idCtrlPort, CSerialPortDialog::OnPortChange)   // DOS only
#endif
END_EVENT_TABLE()


CSerialPortDialog::CSerialPortDialog(wxWindow *_parent, const wxString &title, wxString &_sDefaultPort, int _iDefaultBaud)
    : wxDialog(_parent, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	// Remember the default port as I may need it if
	// I open the file dialog
	m_sSelectedPort = _sDefaultPort;

	/////////////////////////////////////
	// Create control contents
	wxPanel *panel = new wxPanel(this);

	/////////////////////////////////////
	// Now create the top-level sizer.  It is a box
	// that stacks the other sizers vertically.
	// It contains two sizers, one for the controls and another for
	// the buttons
	wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

	// Create a sizer to hold the controls. This flexgridsizer
	// Has three columns - left for the port and baud labels,
	// center for the port name text and baud listbox, and th
	// right (top row) contains the button to open the file
	// dialog for selecting the port from the file system or
	{
		wxStaticText *label;
        wxString baudText;

		wxFlexGridSizer *flexGridSizer = new wxFlexGridSizer(3, 0, 0);
		flexGridSizer->AddGrowableCol(1,0);
		topSizer->Add(flexGridSizer, 1, wxGROW, BORDERPIXELS);

		///////////////////////////
		// Top row

		// Port label
		label = new wxStaticText(panel,-1,_("Port:"),
								 wxDefaultPosition, wxDefaultSize,
								 wxALIGN_CENTER);
		flexGridSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxFIXED_MINSIZE | wxTOP | wxLEFT, BORDERPIXELS);

#ifdef __WXMSW__

		// crate the baud rate list box
		wxArrayString comPortList;
		comPortList.Add("COM1");
		comPortList.Add("COM2");
		comPortList.Add("COM3");
		comPortList.Add("COM4");
		comPortList.Add("COM5");
		comPortList.Add("COM6");
		comPortList.Add("COM7");
		comPortList.Add("COM8");

		m_pPortBoxCtrl = 0;
		m_pPortBoxCtrl = new wxChoice(panel, idCtrlPort, wxDefaultPosition, wxDefaultSize, comPortList);
        m_pPortBoxCtrl->SetSelection(m_pPortBoxCtrl->FindString(_sDefaultPort));
		flexGridSizer->Add (m_pPortBoxCtrl, 1, wxEXPAND | wxTOP | wxLEFT, BORDERPIXELS);

		m_sSelectedPort = m_pPortBoxCtrl->GetString(m_pPortBoxCtrl->GetSelection());

        // Keep apearance consistent
        flexGridSizer->AddStretchSpacer();
#endif

#ifdef __UNIX_LIKE__

		// Port path text control
		m_pDevicePathTextCtrl = 0;
		m_pDevicePathTextCtrl = new wxTextCtrl(panel, idCtrlDevicePathText, _sDefaultPort, wxDefaultPosition, wxSize(100, -1), wxTE_PROCESS_ENTER);
		flexGridSizer->Add (m_pDevicePathTextCtrl, 1, wxEXPAND | wxTOP | wxLEFT, BORDERPIXELS);

		// Finally, the little button with the ... for browsing the file system
		wxButton *browseButton = new wxButton(panel, idCtrlDevicePathBrowse, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
		flexGridSizer->Add(browseButton, 0, wxFIXED_MINSIZE | wxALIGN_RIGHT | wxALIGN_BOTTOM | wxALL | wxBOTTOM, BORDERPIXELS);
#endif
		///////////////////////////
		// Bottom row on Linux, top
		// final entry on top row on
		// DOS

		// This row has the baud rate label and choice
		label = new wxStaticText(panel,-1,_("Baud Rate:"),
								 wxDefaultPosition, wxDefaultSize,
								 wxALIGN_CENTER);
		flexGridSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxFIXED_MINSIZE | wxTOP | wxLEFT, BORDERPIXELS);

		// crate the baud rate list box
		wxArrayString baudRateList;
		baudRateList.Add("4800");
		baudRateList.Add("9600");
		baudRateList.Add("19200");
		baudRateList.Add("38400");
		baudRateList.Add("56000");
		baudRateList.Add("57600");
		baudRateList.Add("115200");

		m_pBaudRateBoxCtrl = 0;
		m_pBaudRateBoxCtrl = new wxChoice(panel, idCtrlBaud, wxDefaultPosition, wxDefaultSize, baudRateList);

		switch(_iDefaultBaud)
		{
			case 4800:		m_pBaudRateBoxCtrl->SetSelection(0);	break;
			default:
			case 9600:		m_pBaudRateBoxCtrl->SetSelection(1);	break;
			case 19200:		m_pBaudRateBoxCtrl->SetSelection(2); 	break;
			case 38400:		m_pBaudRateBoxCtrl->SetSelection(3);	break;
			case 56000:		m_pBaudRateBoxCtrl->SetSelection(4); 	break;
			case 57600:		m_pBaudRateBoxCtrl->SetSelection(5); 	break;
			case 115200:	m_pBaudRateBoxCtrl->SetSelection(6); 	break;
		}

		flexGridSizer->Add (m_pBaudRateBoxCtrl, 1, wxEXPAND | wxTOP | wxLEFT, BORDERPIXELS);

		baudText = m_pBaudRateBoxCtrl->GetString(m_pBaudRateBoxCtrl->GetSelection());
		baudText.ToLong(&m_iBaudRate);
    }

	/////////////////////////////////////////////////////////////////////
	// Create a sizer for the ok, cancel, and calibrate buttons
	{
		wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
		topSizer->Add(buttonSizer, 0, wxFIXED_MINSIZE | wxALIGN_RIGHT);

		wxButton *cancelButton = new wxButton(panel, wxID_CANCEL, _("Cancel"));
		buttonSizer->Add(cancelButton, 1, wxFIXED_MINSIZE | wxTOP | wxLEFT | wxBOTTOM, BORDERPIXELS);

		wxButton *okButton = new wxButton(panel, wxID_OK, _("OK"));
		buttonSizer->Add(okButton, 1, wxFIXED_MINSIZE | wxALL, BORDERPIXELS);
	}

	/////////////////////////////////////
	// Set the layout in motion
	panel->SetAutoLayout(true);
	panel->SetSizer(topSizer);
	topSizer->Fit(this);
	SetMinSize(GetSize());
}

CSerialPortDialog::~CSerialPortDialog()
{

}

void CSerialPortDialog::OnOK(wxCommandEvent &WXUNUSED(_e))
{
    EndModal(wxID_OK);
}

void CSerialPortDialog::OnCancel(wxCommandEvent &WXUNUSED(_e))
{
    EndModal(wxID_CANCEL);
}

void CSerialPortDialog::OnClose(wxCloseEvent &WXUNUSED(_e))
{
    EndModal(wxID_CANCEL);
}

#ifdef __UNIX_LIKE__
void CSerialPortDialog::OnDevicePathTextCtrlChange(wxCommandEvent& WXUNUSED(_e))
{
	if(m_pDevicePathTextCtrl)
		m_sSelectedPort = m_pDevicePathTextCtrl->GetValue();
}

void CSerialPortDialog::OnDevicePathTextCtrlEnter(wxCommandEvent& WXUNUSED(_e))
{
	EndModal(wxID_OK);
}

void CSerialPortDialog::OnDevicePathBrowse(wxCommandEvent& WXUNUSED(_e))
{

	wxFileName filePath(m_sSelectedPort);
	wxString fileDir;
	wxString fileName;

	fileDir = filePath.GetPath();
	fileName = filePath.GetFullName();

	wxFileDialog openFileDialog(this, _("Browse for serial tty port"), fileDir, fileName,
						wxFileSelectorDefaultWildcardStr, wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// Check for cancel button
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;     // the user changed idea...

	// Get the selected file
	m_sSelectedPort = openFileDialog.GetPath();

	// And set it in the text control
	m_pDevicePathTextCtrl->SetValue(m_sSelectedPort);
}
#endif

void CSerialPortDialog::OnBaudChange(wxCommandEvent& WXUNUSED(_e))
{
	wxString baudText = m_pBaudRateBoxCtrl->GetString(m_pBaudRateBoxCtrl->GetSelection());
	baudText.ToLong(&m_iBaudRate);
}

#ifdef __WXMSW__
void CSerialPortDialog::OnPortChange(wxCommandEvent& WXUNUSED(_e))
{
	if(m_pPortBoxCtrl)
		m_sSelectedPort = m_pPortBoxCtrl->GetString(m_pPortBoxCtrl->GetSelection());
}
#endif
