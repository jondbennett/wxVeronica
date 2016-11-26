
#include <wx/wx.h>

#include "../../GaryCooper/TelemetryTags.h"
#include "TelemetryData01.h"

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
	m_extraLightTime = TELEMETRY_BAD_FLOAT;

	// telemetry_tag_command_ack
	m_commandAckTag = TELEMETRY_BAD_INT;

	// telemetry_tag_command_ack
	m_commandNakTag = TELEMETRY_BAD_INT;
	m_commandNakReason = TELEMETRY_BAD_INT;
}

void CTelemetryData_01::processTelemetry(wxArrayString _telemData)
{
	// The first term is the tag
	int tag = atoi(_telemData[0].c_str());

	switch(tag)
	{
	case telemetry_tag_error_flags:
		m_errorCode = atoi(_telemData[1].c_str());
		break;

	case telemetry_tag_GPSStatus:
		m_GPSLocked = (atoi(_telemData[1].c_str())) ? true : false;
		m_nSats = atoi(_telemData[2].c_str());
		m_lat = atof(_telemData[3].c_str());
		m_lon = atof(_telemData[4].c_str());
		break;

	case telemetry_tag_date_time:
		m_year = atoi(_telemData[1].c_str());
		m_month = atoi(_telemData[2].c_str());
		m_day = atoi(_telemData[3].c_str());
		m_currentTime = atof(_telemData[4].c_str());
		break;

	case telemetry_tag_sun_times:
		m_sunriseTime = atof(_telemData[1].c_str());
		m_sunsetTime = atof(_telemData[2].c_str());
		break;

	case telemetry_tag_door_config:
		m_sunriseOffset = atoi(_telemData[1].c_str());
		m_sunsetOffset = atoi(_telemData[2].c_str());
		m_stuckDoorDelay = atoi(_telemData[3].c_str());
		break;

	case telemetry_tag_door_info:
		m_doorOpenTime = atof(_telemData[1].c_str());
		m_doorCloseTime = atof(_telemData[2].c_str());
		m_doorStatus = atoi(_telemData[3].c_str());
		break;

	case telemetry_tag_light_config:
		m_minimumDayLength = atof(_telemData[1].c_str());
		m_extraLightTime = atof(_telemData[2].c_str());
		break;
		break;

	case telemetry_tag_light_info:
		m_morningLightOnTime = atof(_telemData[1].c_str());
		m_morningLightOffTime = atof(_telemData[2].c_str());
		m_eveningLightOnTime = atof(_telemData[3].c_str());
		m_eveningLightOffTime = atof(_telemData[4].c_str());
		m_lightIsOn  = (atoi(_telemData[5].c_str())) ? true : false;
		break;

	case telemetry_tag_command_ack:
		m_commandAckTag = atoi(_telemData[1].c_str());
		m_commandAckValue = atof(_telemData[2].c_str());
		break;

	case telemetry_tag_command_nak:
		m_commandNakTag = atoi(_telemData[1].c_str());
		m_commandNakValue = atof(_telemData[2].c_str());
		m_commandNakReason = atoi(_telemData[3].c_str());

	default:
		break;
	}
}

