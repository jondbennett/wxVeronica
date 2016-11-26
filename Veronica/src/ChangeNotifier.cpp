////////////////////////////////////////////////////////////
// Change Notifier
////////////////////////////////////////////////////////////

#include <wx/wx.h>

#include "ChangeNotifier.h"
#include "SoundThread.h"

////////////////////////////////////////////////////////////
// This CChangeNotifier keeps remembers a provided status, and issues
// a notification whenever the provided status changes. It
// is intended to notify of the loss or restoration of some
// service and present a notification when the service changes.
// As implemented, it provides notification by playing a sound
// file.
////////////////////////////////////////////////////////////
#define RENOTIFY_PERIOD (60L)
#define STATUS_UNKNOWN	((CChangeNotifier_statusT) -1)
static CSoundThread *s_soundThread = 0;

CChangeNotifier::CChangeNotifier(const char *_loss, const char *_restore)
{
	m_previousStatus = STATUS_UNKNOWN;
	m_timeOfLastNotification = 0;

	m_loss = wxString("Sounds/") + wxString(_loss);
	m_restore = wxString("Sounds/") + wxString(_restore);

	m_muteLoss = false;
	m_muteRestore = false;
}

CChangeNotifier::~CChangeNotifier()
{

}

void CChangeNotifier::mute(bool _muteLoss, bool _muteRestore)
{
	m_muteLoss = _muteLoss;
	m_muteRestore = _muteRestore;
}

void CChangeNotifier::status(CChangeNotifier_statusT _newStatus, CChangeNotifier_forceT _force)
{
	CChangeNotifier_statusT oldStatus = m_previousStatus;
	bool playNotification = (CChangeNotifier_force_always == _force)? true : false;

	// Figure out if we are going to play it
	if(oldStatus ==  STATUS_UNKNOWN)
	{
		if(_force == CChangeNotifier_force_ifFirst)
			playNotification = true;

		if(_newStatus == CChangeNotifier_status_loss)
			playNotification = true;
	}
	else
	{
		if(oldStatus != _newStatus)
			playNotification = true;

		if((oldStatus == _newStatus) &&
			(CChangeNotifier_force_periodic == _force) &&
			(wxGetLocalTime() > m_timeOfLastNotification + RENOTIFY_PERIOD))
			playNotification = true;

	}

	// Play the notification
	if(playNotification)
	{
		m_timeOfLastNotification = wxGetLocalTime();

		if(! m_muteLoss && (_newStatus == CChangeNotifier_status_loss && m_loss.length()))
			playSound(m_loss);

		if(!m_muteRestore && (_newStatus == CChangeNotifier_status_restore && m_restore.length()))
			playSound(m_restore);
	}

	// Remember the new state
	m_previousStatus = _newStatus;
}

void CChangeNotifier::status(bool _newStatus, CChangeNotifier_forceT _force)
{
	status(_newStatus ? CChangeNotifier_status_restore : CChangeNotifier_status_loss, _force);
}

void CChangeNotifier::playSound(wxString &_fileName)
{
	// Create the sound thread if needed
	if(s_soundThread == 0)
	{
		s_soundThread = new CSoundThread;
		s_soundThread->Run();
		wxSleep(1);
	}

	// Now, queue the sound
	s_soundThread->QueueSound(_fileName);
}
