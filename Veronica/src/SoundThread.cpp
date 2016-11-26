////////////////////////////////////////////////////////////
// SoundThread
////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/sound.h>

#include "SoundThread.h"

////////////////////////////////////////////////////////////
// Use a wxArrayString to queue sounds for playing with
// a "system" command. This avoids the problem of having
// the main GUI wait while the sound plays.
// Uses a semaphore for synchronization.
////////////////////////////////////////////////////////////
CSoundThread::CSoundThread()
{
}

CSoundThread::~CSoundThread()
{
}

void *CSoundThread::Entry()
{
	wxString soundFileName;

	while(1)
	{
		// Wait here until I am given a chance to run
		m_runSemaphore.Wait();

		// Remove a name from the queue
		soundFileName = wxEmptyString;
		{
			wxCriticalSectionLocker csl(m_listLock);
			if(m_fileList.GetCount() > 0)
			{
				soundFileName = m_fileList[0];
				m_fileList.RemoveAt(0);
			}
		}

		// Play the sound
		if(soundFileName.length())
			playSound(soundFileName);
	}

	// Never reached
	return 0;
}

void CSoundThread::QueueSound(wxString &_filePath)
{
	wxCriticalSectionLocker csl(m_listLock);

	m_fileList.Add(_filePath);
	m_runSemaphore.Post();
}

#ifdef __UNIX_LIKE__
static const char *s_playCommand = "paplay";
#endif

void CSoundThread::playSound(wxString &_fileName)
{
	if(_fileName.length() == 0)
		return;

#ifdef __UNIX_LIKE__
	wxString playCommand = wxString::Format("%s %s",	s_playCommand, _fileName.c_str());
	if(playCommand.length())
		system(playCommand.c_str());
#endif

#ifdef __WXMSW__
	wxSound::Play(_fileName, wxSOUND_SYNC);
#endif

}

