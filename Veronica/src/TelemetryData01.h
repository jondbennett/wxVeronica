

class CTelemetryData_01
{
public:
	// telemetry_tag_error
	int m_errorCode;

	// telemetry_tag_GPSStatus
	bool m_GPSLocked;
	int m_nSats;
	double m_lat;
	double m_lon;

	//telemetry_tag_date_time
	int m_year;
	int m_month;
	int m_day;
	double m_currentTime;

	// telemetry_tag_sun_times
	double m_sunriseTime;
	double m_sunsetTime;

	// telemetry_tag_door_config
	int m_sunriseOffset;
	int m_sunsetOffset;
	int m_stuckDoorDelay;

	// telemetry_tag_door_info
	double m_doorOpenTime;
	double m_doorCloseTime;
	int m_doorStatus;

	// telemetry_tag_light_config
	double m_minimumDayLength;
	double m_extraLightTime;

	// telemetry_tag_light_info
	double m_morningLightOnTime;
	double m_morningLightOffTime;
	double m_eveningLightOnTime;
	double m_eveningLightOffTime;
	bool m_lightIsOn;

	// telemetry_tag_command_ack
	int m_commandAckTag;
	double m_commandAckValue;

	// telemetry_tag_command_nak
	int m_commandNakTag;
	double m_commandNakValue;
	int m_commandNakReason;

	CTelemetryData_01();

	void clearGPSRelated();
	void clearCommsRelated();

	void processTelemetry(wxArrayString _telemData);
};

#define TELEMETRY_BAD_INT	(-1)
#define TELEMETRY_BAD_FLOAT	(-999.)
