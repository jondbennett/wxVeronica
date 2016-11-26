////////////////////////////////////////////////////////////
// CFreezer - Freeze / Thaw class to reduce flicker
////////////////////////////////////////////////////////////
#ifndef FREEZER_H
#define FREEZER_H

class CFreezer
{
	private:
	wxWindow *m_w;

	public:
	CFreezer(wxWindow *_w)	// Freeze in the constructor
	{
		m_w = _w;
		if(m_w)
			m_w->Freeze();
	}

	~CFreezer()				// Thaw in the destructor
	{
		if(m_w)
			m_w->Thaw();
	}
};

#endif
