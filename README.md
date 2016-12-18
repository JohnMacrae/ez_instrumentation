# ez_instrumentation
ESP8266 temperature reporting module MQTT

This code was built for The Hoveraid Trust (http://uk.hoveraid.org/). They needed a bearing temperature monitoring system for their hovercraft that operate in remote areas.
I'm a newbie at embedded programming - the code probably contains a buch of errors - that said, it works reliably.

Features:

	Uses up to 6 DS18b20 temperature sensors (possibly more).
	Reports individual sensor temperatures with sensor serial number via MQTT.
	The temperature is reported as Host/<sensor_ID>/temperature (C)	

Commands:

	The system responds to the following commands via serial.  It returns values via serial
	The default hostname is 'Default'. It can be changed by using the Host Command below.

The syntax is always Command:: as below.  Commands are case sensitive.

	resetsettings::			Resets the settings in spiffs to default values
	BrokerIP::xxx.xxx.xxx.xxx	Sets the MQTT broker IP address
	WiFi:SSID:Pass			Sets the SSID and password 
	Delay::delay (ms)		Sets the Delay between temperature readings
	Reboot::			Reboots the system
	PWM:GPIO:DutyCycle(0-1023)	Sets a GPIO to PWM
	GPIO:GPIO:1 or 0		Sets a GPIO on or off
	PrintSettings			Prints the settings (to MQTT Host/outTopic)
	PrintSensors			Prints the sensor IDs (to MQTT Host/outTopic)
	Host::NewHostname. 		Sets the new Hostname

Obviously several of these will require the setting to be changed and then issue a Reboot command.

