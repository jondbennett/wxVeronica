
#include <wx/wx.h>

#include "../../../GaryCooper/TelemetryTags.h"
#include "TelemetryData01.h"

////////////////////////////////////////////////////////////////
// Check count in wxArrayString data to make sure we have
// enough data to avoid a bounds check
static bool countFail(wxArrayString _as,
						size_t _min,
						const char *_fileName,
						int _lineNo)
{
	if(_as.GetCount() < _min)
	{
		wxString msg;

		msg = wxString::Format("Too few telemetry fields at %s : %d.\n"
								"Needed %d, received %s.",
								_fileName, _lineNo,
								_min, _as.GetCount());
		return true;
	}

	return false;
}

#define COUNTFAIL(x)	(countFail(_telemData, x, __FILE__, __LINE__))
////////////////////////////////////////////////////////////////

CTelemetryData_01::CTelemetryData_01()
{
	clearGPSRelated();
	clearCommsRelated();
	m_lightIsOn = false;
}

void CTelemetryData_01::clearGPSRelated()
{
	// telemetry_tag_error
	m_errorCode = telemetry_error_no_error;

	// telemetry_tag_GPSStatus
	m_GPSLocked = false;
	m_nSats = TELEMETRY_BAD_INT;
	m_lat = m_lon = TELEMETRY_BAD_FLOAT;

	//telemetry_tag_date_time
	m_year = TELEMETRY_BAD_INT;
	m_month = TELEMETRY_BAD_INT;
	m_day = TELEMETRY_BAD_INT;
	m_currentTime = TELEMETRY_BAD_FLOAT;

	//telemetry_tag_sun_times
	m_sunriseTime = TELEMETRY_BAD_FLOAT;
	m_sunsetTime = TELEMETRY_BAD_FLOAT;

	// telemetry_tag_door_info
	m_doorOpenTime = TELEMETRY_BAD_FLOAT;
	m_doorCloseTime = TELEMETRY_BAD_FLOAT;

	// telemetry_tag_light_info
	m_morningLightOnTime = TELEMETRY_BAD_FLOAT;
	m_morningLightOffTime = TELEMETRY_BAD_FLOAT;
	m_eveningLightOnTime = TELEMETRY_BAD_FLOAT;
	m_eveningLightOffTime = TELEMETRY_BAD_FLOAT;
}

void CTelemetryData_01::clearCommsRelated()
{
	// telemetry_tag_door_config
	m_sunriseOffset = TELEMETRY_BAD_INT;
	m_sunsetOffset = TELEMETRY_BAD_INT;
	m_doorStatus = TELEMETRY_BAD_INT;

	// telemetry_tag_light_config
	m_minimumDayLength = TELEMETRY_BAD_FLOAT;
	m_extraLightTimeMorning = TELEMETRY_BAD_FLOAT;
	m_extraLightTimeEvening = TELEMETRY_BAD_FLOAT;

	// telemetry_tag_command_ack
	m_commandAckTag = TELEMETRY_BAD_INT;

	// telemetry_tag_command_ack
	m_commandNakTag = TELEMETRY_BAD_INT;
	m_commandNakReason = TELEMETRY_BAD_INT;
}

void CTelemetryData_01::processTelemetry(wxArrayString _telemData)
{
	// Sanity
	if(COUNTFAIL(1))
		return;

	// The first term is the tag
	int tag = atoi(_telemData[0].c_str());

	switch(tag)
	{
	case telemetry_tag_error_flags:
		if(COUNTFAIL(2)) break;

		m_errorCode = atoi(_telemData[1].c_str());
		break;

	case telemetry_tag_GPSStatus:
		if(COUNTFAIL(5)) break;

		m_GPSLocked = (atoi(_telemData[1].c_str())) ? true : false;
		m_nSats = atoi(_telemData[2].c_str());
		m_lat = atof(_telemData[3].c_str());
		m_lon = atof(_telemData[4].c_str());
		break;

	case telemetry_tag_date_time:
		if(COUNTFAIL(5)) break;

		m_year = atoi(_telemData[1].c_str());
		m_month = atoi(_telemData[2].c_str());
		m_day = atoi(_telemData[3].c_str());
		m_currentTime = atof(_telemData[4].c_str());
		break;

	case telemetry_tag_sun_times:
		if(COUNTFAIL(3)) break;

		m_sunriseTime = atof(_telemData[1].c_str());
		m_sunsetTime = atof(_telemData[2].c_str());
		break;

	case telemetry_tag_door_config:
		if(COUNTFAIL(4)) break;

		m_sunriseOffset = atoi(_telemData[1].c_str());
		m_sunsetOffset = atoi(_telemData[2].c_str());
		m_stuckDoorDelay = atoi(_telemData[3].c_str());
		break;

	case telemetry_tag_door_info:
		if(COUNTFAIL(4)) break;

		m_doorOpenTime = atof(_telemData[1].c_str());
		m_doorCloseTime = atof(_telemData[2].c_str());
		m_doorStatus = atoi(_telemData[3].c_str());
		break;

	case telemetry_tag_light_config:
		if(COUNTFAIL(4)) break;

		m_minimumDayLength = atof(_telemData[1].c_str());
		m_extraLightTimeMorning = atof(_telemData[2].c_str());
		m_extraLightTimeEvening = atof(_telemData[3].c_str());
		break;

	case telemetry_tag_light_info:
		if(COUNTFAIL(6)) break;

		m_morningLightOnTime = atof(_telemData[1].c_str());
		m_morningLightOffTime = atof(_telemData[2].c_str());
		m_eveningLightOnTime = atof(_telemData[3].c_str());
		m_eveningLightOffTime = atof(_telemData[4].c_str());
		m_lightIsOn  = (atoi(_telemData[5].c_str())) ? true : false;
		break;

	case telemetry_tag_command_ack:
		if(COUNTFAIL(3)) break;

		m_commandAckTag = atoi(_telemData[1].c_str());
		m_commandAckValue = atof(_telemData[2].c_str());
		break;

	case telemetry_tag_command_nak:
		if(COUNTFAIL(4)) break;

		m_commandNakTag = atoi(_telemData[1].c_str());
		m_commandNakValue = atof(_telemData[2].c_str());
		m_commandNakReason = atoi(_telemData[3].c_str());

	default:
		break;
	}
}

