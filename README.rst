rad-esp8266
===========

.. image:: https://travis-ci.org/randuhmm/rad-esp8266.svg?branch=master
  :target: https://travis-ci.org/randuhmm/rad-esp8266
  :alt: Latest Travis CI build status

.. image:: https://readthedocs.org/projects/rad-esp8266/badge/?version=latest
  :target: http://rad-esp8266.readthedocs.io/en/latest/?badge=latest
  :alt: Documentation Status

Documentation
-------------

The full documentation is at http://rad-ESP8266.rtfd.org.

Overview
--------

An IoT framework library for ESP8266 modules.

This library builds on top of existing libraries such as WiFiManager, ArduionJson and the ESP8266-Core to provide a complete framework for building, configuring, discovering and communicating with a variety of devices such as sensors and switches.

Many "barebones" examples are provided that can simply be flashed to a chip and configured. For simple switch and sensor applications, these may provide all the functionality that is needed, however, the API can also be used to customize the devices for more complex or custom applications.

Requirements
------------

**Supported ESP8266 chip versions:**

* ESP-01
* ESP-12E

**Supported Arduino IDE versions:**

* Arduino IDE >= 1.6.4

**Supported Arduino ESP8266 versions:**

* ESP8266 Core 2.X.X

**Supported AduinoJson versions:**

* ArduinoJson 5.10.0

**Supported LinkedList versions:**

* LinkedList 1.2.3

**Supported WiFiManager versions:**

* WiFiManager 0.12.0

Quickstart With Arduino IDE
---------------------------

1. Install the latest version of the Arduino IDE located here: https://www.arduino.cc/en/Main/Software

2. Install the ESP8266 Core package using the Boards Manager. You can follow the instructions from the GitHub or the steps below should work as well. https://github.com/esp8266/Arduino

  * Start Arduino and select ``File > Preferences`` from the application menu.
  * Enter the URL ``http://arduino.esp8266.com/stable/package_esp8266com_index.json`` into the ``Additional Board Manager URLs`` field. You can add multiple URLs, separating them with commas.
  * Open the Boards Manager from ``Tools > Board`` menu and install esp8266 platform (and don't forget to select your ESP8266 board from ``Tools > Board`` menu after installation).

3. Select ``Sketch > Include Library > Manage Libraries`` from the application menu to open the Library Manager. Search for and install the latest supported versions of the following libraries:

  * ``ArduinoJson``
  * ``LinkedList``
  * ``WiFiManager``

4. Download the ``RadEsp8266`` library and extract it to your ``libraries`` directory for the Arduino IDE.

5. From the application menu ``File > Examples > RadEsp8266``, locate the and open the ``SimpleBinarySwitch`` sketch and add your local WiFi credentials::

     const char* ssid = "YOUR_SSID";
     const char* pass = "YOUR_PASSWORD";

6. Be sure to set the proper upload settings in the ``Tools`` menu and then upload the sketch to the module.

7. Once sucessfully uploaded, connect a momentary push button to ``GPIO 0`` and a LED to ``GPIO 2``. See the following connection diagram for a reference.

8. If you see the following output from the Serial port, your module is sucessfully connected to your WiFi and ready to receive commands and send events::

     Starting...


     Wait for WiFi... 
     WiFi connected
     IP address: 
     192.168.1.113

9. You should be able to push the momentary button to toggle the LED. Using a computer or device located on the same network as the ESP8266, you can submit an HTTP request like the following to turn the LED on or off::

     # cURL request to turn the LED on
     curl -X POST 'http://<IP_ADDRESS>:8080/devices/switch_1/commands' --data '{"type": "Set", "data": {"value": true}}'

     # cURL request to turn the LED off
     curl -X POST 'http://<IP_ADDRESS>:8080/devices/switch_1/commands' --data '{"type": "Set", "data": {"value": false}}'

