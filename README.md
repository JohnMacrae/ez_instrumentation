# ez_instrumentation
ESP8266 temperature reporting module MQTT

This code was built for The Hoveraid Trust (http://uk.hoveraid.org/). They needed a bearing temperature monitoring system for their hovercraft that operate in remote areas.
I'm a newbie at embedded programming - the code probably contains a buch of errors - that said, it works reliably.

Features:
Uses up to 6 DS18b20 temperature sensors (possibly more).
reports individual sensor temperatures with sensor serial number.

Commands:
The 

resetsettings::
BrokerIP::xxx.xxx.xxx.xxx
WiFi:SSID:Pass
Broker:BrokerUser:BrokerPass
Delay::delay (ms)
Reboot::
PWM:GPIO:DutyCycle(0-1023)
GPIO:GPIO:1 or 0
PrintSettings
PrintSensors
