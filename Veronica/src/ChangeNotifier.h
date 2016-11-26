////////////////////////////////////////////////////////////
// Change Notifier
////////////////////////////////////////////////////////////
#ifndef ChangeNotifier_h
#define ChangeNotifier_h

////////////////////////////////////////////////////////////
// This class keeps remembers a provided status, and issues
// a notification whenever the provided status changes. It
// is intended to notify of the loss or restoration of some
// service and present a notification when the service changes.
// As implemented, it provides notification by playing a sound
// file.
////////////////////////////////////////////////////////////
typedef enum
{
	CChangeNotifier_status_loss = 0,
	CChangeNotifier_status_restore,
} CChangeNotifier_statusT;

typedef enum
{
	CChangeNotifier_force_none,
	CChangeNotifier_force_ifFirst,
	CChangeNotifier_force_always,
	CChangeNotifier_force_periodic,
} CChangeNotifier_forceT;

class CChangeNotifier
{
protected:
	CChangeNotifier_statusT m_previousStatus;

	wxString m_loss;
	wxString m_restore;
	long m_timeOfLastNotification;

	bool m_muteLoss;
	bool m_muteRestore;

	void playSound(wxString &_fileName);

public:
	CChangeNotifier(const char *_loss, const char *_restore);
	virtual ~CChangeNotifier();

	void mute(bool _muteLoss, bool _muteRestore);

	void status(CChangeNotifier_statusT _newStatus, CChangeNotifier_forceT _force = CChangeNotifier_force_ifFirst);
	void status(bool _newStatus, CChangeNotifier_forceT _force = CChangeNotifier_force_ifFirst);
};

#endif
