////////////////////////////////////////////////////////////
// SoundThread
////////////////////////////////////////////////////////////
#ifndef SoundThread_h
#define SoundThread_h

////////////////////////////////////////////////////////////
// Use a wxArrayString to queue sounds for playing with
// a "system" command. This avoids the problem of having
// the main GUI wait while the sound plays.
////////////////////////////////////////////////////////////
class CSoundThread : public wxThread
{
protected:

	wxSemaphore m_runSemaphore;

	wxCriticalSection m_listLock;
	wxArrayString m_fileList;

	void *Entry();
	void playSound(wxString &_fileName);

public:
	CSoundThread();
	virtual ~CSoundThread();

	void QueueSound(wxString &_filePath);
};

#endif
