#include <wx/wx.h>

#include "ConfigKeys.h"

// These are external strings that provide the
// keys to the config database.
wxString config_key_serial_port("/Serial/PortName");
wxString config_key_serial_baud("/Serial/Baudrate");

wxString config_key_voice_alert_door_open("/Voice/Alert_Door_Open");
wxString config_key_voice_alert_door_close("/Voice/Alert_Door_Close");

wxString config_key_voice_alert_light_on("/Voice/Alert_Light_On");
wxString config_key_voice_alert_light_off("/Voice/Alert_Light_Off");

wxString config_key_voice_warning_comm("/Voice/Warning_Comm");
wxString config_key_voice_warning_gps("/Voice/Warning_GPS");
