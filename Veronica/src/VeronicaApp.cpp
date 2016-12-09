/***************************************************************
 * Name:      VeronicaApp.cpp
 * Purpose:   Code for Application Class
 * Author:    Jon Bennett (jon@jondbennett.com)
 * Created:   2016-11-18
 * Copyright: Jon Bennett (http://jondbennett.com)
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include <wx/wx.h>
#include <wx/config.h>

#include "../../../GaryCooper/ICommInterface.h"
#include "../../../GaryCooper/Telemetry.h"
#include "../../../GaryCooper/TelemetryTags.h"

#include "SerialComm.h"
#include "SerialPortDialog.h"
#include "TelemetryData01.h"
#include "CommandSender.h"

#include "VeronicaApp.h"
#include "VeronicaMain.h"

IMPLEMENT_APP(VeronicaApp);

bool VeronicaApp::OnInit()
{
	wxConfig *config = new wxConfig("Veronica");
	config->SetPath(wxString("/"));

    VeronicaDialog* dlg = new VeronicaDialog(0L, _("Veronica Cooper - Monitor"));

    dlg->Show();
    return true;
}
