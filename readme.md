# ESP8266 / ESP32 to MQTT

This project is based of https://github.com/roarfred/AmsToMqttBridge HanReader code.

I wanted to have dynamic parsing of the messages, as well as applying the unit and scaler to go from: 2404, Scaler: 0.1 -> 240.4V before sending message on mqtt.
I also get the current time from a NTP-server so all list updates have the time unix epoch.

On the subscription end of MQTT you can use list_updated as a syncronization point and update the topics that have a matching timestamp.
The units is stored in a persistent topic and you will get them on subscription/connection.

I'm currently hard coding the secrets (ssid/psk/server adresses/...) in a file secret.h,
I've included an example of this file.

The raw M-Bus data is also sent over MQTT on a "raw/powermeterhan" topic.

# !!! Known Issues !!!

## Instability/Corruption [Status: Mitigated]

There is an instability-issue where the system either loses connection or does not receive new HAN-data after a weeks time or so.
Does not look to be an OOM issue. Currently there's code in now that triggers a watchdog if no new HAN-message is received in 60 seconds, and restarts the device. I'm gathering more log samples to track down this issue.


# MQTT Messages

Static information is stored as persistent messages. I.e. units.

# Static information (sent on subscription)
```
[2019-03-26 18:45:18.065018] kdg/sensors/powermeterhan/obis/1.0.1.7.0.255/unit W
[2019-03-26 18:45:18.070296] kdg/sensors/powermeterhan/obis/1.1.0.2.129.255/unit TYPE_STRING
[2019-03-26 18:45:18.070409] kdg/sensors/powermeterhan/obis/0.0.96.1.0.255/unit TYPE_STRING
[2019-03-26 18:45:18.070449] kdg/sensors/powermeterhan/obis/0.0.96.1.7.255/unit TYPE_STRING
[2019-03-26 18:45:18.065183] kdg/sensors/powermeterhan/obis/1.0.2.7.0.255/unit W
[2019-03-26 18:45:18.065322] kdg/sensors/powermeterhan/obis/1.0.3.7.0.255/unit var
[2019-03-26 18:45:18.065476] kdg/sensors/powermeterhan/obis/1.0.4.7.0.255/unit var
[2019-03-26 18:45:18.065614] kdg/sensors/powermeterhan/obis/1.0.31.7.0.255/unit A
[2019-03-26 18:45:18.065750] kdg/sensors/powermeterhan/obis/1.0.71.7.0.255/unit A
[2019-03-26 18:45:18.065800] kdg/sensors/powermeterhan/obis/1.0.32.7.0.255/unit V
[2019-03-26 18:45:18.065928] kdg/sensors/powermeterhan/obis/1.0.52.7.0.255/unit V
[2019-03-26 18:45:18.066065] kdg/sensors/powermeterhan/obis/1.0.72.7.0.255/unit V
[2019-03-26 18:45:18.066202] kdg/sensors/powermeterhan/obis/0.0.1.0.0.255/unit TYPE_DATETIME
[2019-03-26 18:45:18.066339] kdg/sensors/powermeterhan/obis/1.0.1.8.0.255/unit Wh
[2019-03-26 18:45:18.066489] kdg/sensors/powermeterhan/obis/1.0.2.8.0.255/unit Wh
[2019-03-26 18:45:18.066627] kdg/sensors/powermeterhan/obis/1.0.3.8.0.255/unit varh
[2019-03-26 18:45:18.066764] kdg/sensors/powermeterhan/obis/1.0.4.8.0.255/unit varh

```

So an update looks like:

# List 1
```
 [2019-03-26 18:23:18.121218] kdg/sensors/powermeterhan/obis/1.0.1.7.0.255/value 875
 [2019-03-26 18:23:18.122672] kdg/sensors/powermeterhan/obis/1.0.1.7.0.255/updated 1553624597
 [2019-03-26 18:23:18.123568] kdg/sensors/powermeterhan/obis/list_updated 1553624597
```
# List 2
```
 [2019-03-26 18:23:23.384712] kdg/sensors/powermeterhan/obis/1.1.0.2.129.255/value AIDON_V0001
 [2019-03-26 18:23:23.385372] kdg/sensors/powermeterhan/obis/1.1.0.2.129.255/updated 1553624602
 [2019-03-26 18:23:23.387694] kdg/sensors/powermeterhan/obis/0.0.96.1.0.255/value 00000000000000
 [2019-03-26 18:23:23.388073] kdg/sensors/powermeterhan/obis/0.0.96.1.0.255/updated 1553624602
 [2019-03-26 18:23:23.389235] kdg/sensors/powermeterhan/obis/0.0.96.1.7.255/value 6525
 [2019-03-26 18:23:23.390429] kdg/sensors/powermeterhan/obis/0.0.96.1.7.255/updated 1553624602
 [2019-03-26 18:23:23.392458] kdg/sensors/powermeterhan/obis/1.0.1.7.0.255/value 881
 [2019-03-26 18:23:23.393056] kdg/sensors/powermeterhan/obis/1.0.1.7.0.255/updated 1553624602
 [2019-03-26 18:23:23.394659] kdg/sensors/powermeterhan/obis/1.0.2.7.0.255/value 0
 [2019-03-26 18:23:23.395707] kdg/sensors/powermeterhan/obis/1.0.2.7.0.255/updated 1553624602
 [2019-03-26 18:23:23.397266] kdg/sensors/powermeterhan/obis/1.0.3.7.0.255/value 0
 [2019-03-26 18:23:23.398283] kdg/sensors/powermeterhan/obis/1.0.3.7.0.255/updated 1553624602
 [2019-03-26 18:23:23.399976] kdg/sensors/powermeterhan/obis/1.0.4.7.0.255/value 268
 [2019-03-26 18:23:23.400769] kdg/sensors/powermeterhan/obis/1.0.4.7.0.255/updated 1553624602
 [2019-03-26 18:23:23.402796] kdg/sensors/powermeterhan/obis/1.0.31.7.0.255/value 3.400000
 [2019-03-26 18:23:23.403777] kdg/sensors/powermeterhan/obis/1.0.31.7.0.255/updated 1553624602
 [2019-03-26 18:23:24.674242] kdg/sensors/powermeterhan/obis/1.0.71.7.0.255/value 1.100000
 [2019-03-26 18:23:24.675265] kdg/sensors/powermeterhan/obis/1.0.71.7.0.255/updated 1553624602
 [2019-03-26 18:23:24.677437] kdg/sensors/powermeterhan/obis/1.0.32.7.0.255/value 225.899994
 [2019-03-26 18:23:24.682557] kdg/sensors/powermeterhan/obis/1.0.32.7.0.255/updated 1553624602
 [2019-03-26 18:23:24.683282] kdg/sensors/powermeterhan/obis/1.0.52.7.0.255/value 228.000000
 [2019-03-26 18:23:24.684645] kdg/sensors/powermeterhan/obis/1.0.52.7.0.255/updated 1553624602
 [2019-03-26 18:23:24.687187] kdg/sensors/powermeterhan/obis/1.0.72.7.0.255/value 227.899994
 [2019-03-26 18:23:24.692937] kdg/sensors/powermeterhan/obis/1.0.72.7.0.255/updated 1553624602
 [2019-03-26 18:23:24.694179] kdg/sensors/powermeterhan/obis/list_updated 1553624602
```
...

