////////////////////////////////////////////////////////////
// CommandSender
////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/listimpl.cpp>

#include "../../../GaryCooper/ICommInterface.h"
#include "../../../GaryCooper/Telemetry.h"
#include "../../../GaryCooper/TelemetryTags.h"

#include "Misc.h"

#include "SerialComm.h"
#include "CommandSender.h"

WX_DEFINE_LIST(CCommandList);

////////////////////////////////////////////////////////////
// More Description and notes
////////////////////////////////////////////////////////////
static long s_maxAge = (CCommandSender_Retry_Delay * CCommandSender_Max_Retries);
CCommandSender::CCommandSender(CTelemetry *_telemetry)
{
	m_telemetry = _telemetry;
}

CCommandSender::~CCommandSender()
{

}

void CCommandSender::sendCommand(CCommandSenderCommand *_c)
{
	if(!m_telemetry)
		return;

	m_telemetry->transmissionStart();
	m_telemetry->sendTerm(_c->m_tag);
	m_telemetry->sendTerm(_c->m_value);
	m_telemetry->transmissionEnd();
}

void CCommandSender::sendCommand(int _tag, double _value)
{
	CCommandSenderCommand *command = new CCommandSenderCommand;
	command->m_tag = _tag;
	command->m_value = _value;

	m_commandList.Append(command);
}

void CCommandSender::gotAck(int _tag, double _value)
{
	// Find a matching command and remove it from the list
	CCommandList::iterator iter;
	for (iter = m_commandList.begin(); iter != m_commandList.end(); ++iter)
	{
		CCommandSenderCommand *command = *iter;

		// If this matches remove it from the list
		// so I don't keep sending it.
		if((command->m_tag == _tag) &&
			(EFFECTIVELY_EQUAL(command->m_value, _value)) &&
			(command->m_lastSent > 0))
		{
			// Dump it
			m_commandList.DeleteObject(command);

			// List modified - iterator invalid. Leave!!
			break;
		}
	}
}

void CCommandSender::gotNak(int _tag, double _value)
{
	gotAck(_tag, _value);
}

void CCommandSender::tick()
{
	// Don't waste any more time
	if(m_commandList.GetCount() == 0)
		return;

	// Send anything that needs to be sent
	time_t currentTime = time(0L);

	// Clear the queue of any commands that we give up on.
	// Probably because NAK processing is not implemented
	CCommandList removeList;
	CCommandList::iterator iter;
	for (iter = m_commandList.begin(); iter != m_commandList.end(); ++iter)
	{
		CCommandSenderCommand *command = *iter;

		// Is it time to give up on it?
		if((command->m_lastSent - command->m_firstSent) > s_maxAge)
		{
			// This has to be done in two passed because it is
			// very bad practice to remove elements from a list
			// while iterating over it.
			removeList.Append(command);
		}
	}

	for (iter = removeList.begin(); iter != removeList.end(); ++iter)
	{
		CCommandSenderCommand *oldCommand = *iter;
		m_commandList.DeleteObject(oldCommand);
	}

	// Send the commands remaining in the queue
	for (iter = m_commandList.begin(); iter != m_commandList.end(); ++iter)
	{
		CCommandSenderCommand *command = *iter;

		// Is it time to re-send it?
		if(currentTime > (command->m_lastSent + CCommandSender_Retry_Delay))
		{
			// Update the time fields
			if(command->m_firstSent == 0)
				command->m_firstSent = currentTime;
			command->m_lastSent = currentTime;
			sendCommand(command);

			// Don't hammer the serial port, the buffer on
			// the arduino is very small
			break;
		}
	}
}
