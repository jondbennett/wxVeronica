////////////////////////////////////////////////////////////
// CommandSender
////////////////////////////////////////////////////////////
#ifndef CommandSender_h
#define CommandSender_h

////////////////////////////////////////////////////////////
// This class sends Gary Cooper commands and waits for ack
// responses. If, after a short time, no ack is received then
// the command is sent again.
////////////////////////////////////////////////////////////
class CCommandSenderCommand
{
public:
	time_t m_firstSent;
	time_t m_lastSent;

	int m_tag;
	double m_value;

	CCommandSenderCommand()
	{
		m_firstSent = 0L;
		m_lastSent = 0L;
		m_tag = 0;
		m_value = 0.;
	}
};

WX_DECLARE_LIST(CCommandSenderCommand, CCommandList);

class CCommandSender
{
protected:
	CTelemetry *m_telemetry;
	CCommandList m_commandList;

	void sendCommand(CCommandSenderCommand *_c);

public:
	CCommandSender(CTelemetry *_telemetry);
	virtual ~CCommandSender();

	void sendCommand(int _tag, double _value);
	void gotAck(int _tag, double _value);
	void gotNak(int _tag, double _value);

	void tick();	// Poll for re-sending
};

#define CCommandSender_Retry_Delay	(10)	// seconds
#define CCommandSender_Max_Retries	(6)		// Seems about right
#endif
