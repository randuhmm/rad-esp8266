API Specification v1.0
======================


.. http:get:: /

   Get info about the RAD-ESP8266 device

   **Example request**:

   .. sourcecode:: http

      GET / HTTP/1.1
      Host: example.com
      Content-Type: application/json

   **Example response**:

   .. sourcecode:: http

      HTTP/1.1 200 OK
      Content-Type: application/json

      {
          "name": "My RAD Device",
          "type": "urn:rad:device:esp8266:1",
          "model": "RAD-ESP8266",
          "description": "Rad ESP8266 WiFi Module for IoT Integration",
          "serial": "ABD123",
          "UDN": "38323636-4558-4dda-9188-cd1234567890"
      }

   :>json string name: The name of the ESP8266
   :>json string type: The full URN
   :>json string model:
   :>json string description:
   :>json string serial:
   :>json string UDN:
   :status 200: no error
   :status 500: error


Features
--------

.. http:get:: /features

   Get a list of device features

   **Example request**:

   .. sourcecode:: http

      GET /features HTTP/1.1
      Host: example.com
      Content-Type: application/json

   **Example response**:

   .. sourcecode:: http

      HTTP/1.1 200 OK
      Content-Type: application/json

      [
          {
              "feature_name": "switch_1",
              "feature_type": "BinarySwitch"
          },
          {
              "feature_name": "switch_2",
              "feature_type": "BinarySwitch"
          }
      ]

   :>jsonarr string name: The feature name
   :>jsonarr string type: The feature type
   :status 200: no error
   :status 500: error


Commands
--------

.. http:post:: /commands

   Create a new command

   **Example request**:

   .. sourcecode:: http

      GET /commands HTTP/1.1
      Host: example.com
      Content-Type: application/json

      {
          "feature_name": "switch_1",
          "command_type": "Set",
          "data": {
              "value": true
          }
      }

   **Example response**:

   .. sourcecode:: http

      HTTP/1.1 200 OK
      Content-Type: text/javascript

   :<json string feature_name: The name of the target feature
   :<json string command_type: The type of command
   :<json object data: The data for the command
   :status 200: no error
   :status 400: when form parameters are missing


Subscriptions
-------------


.. http:get:: /subscriptions

   Get a list of subscriptions

   **Example request**:

   .. sourcecode:: http

      GET /subscriptions HTTP/1.1
      Host: example.com
      Content-Type: application/json

   **Example response**:

   .. sourcecode:: http

      HTTP/1.1 200 OK
      Content-Type: application/json

      [
          {
              "feature_name": "switch_1",
              "event_type": "State",
              "callback": "http://my-server.local:8000/notify",
              "timeout": 3600,
              "duration": 250,
              "calls": 10,
              "errors": 0
          },
          {
              "feature_name": "switch_2",
              "event_type": "State",
              "callback": "http://my-server.local:8000/notify",
              "timeout": 3600,
              "duration": 3000,
              "calls": 200,
              "errors": 1
          }
      ]

   :>jsonarr string feature_name: The name of the target feature
   :>jsonarr string event_type: The type of event
   :>jsonarr string callback: The HTTP callback
   :>jsonarr int timeout: The timeout value
   :>jsonarr int duration: The duration of this subscription
   :>jsonarr int calls: The number of times the event fired
   :>jsonarr int errors: The number of errors
   :status 200: no error
   :status 500: error

.. http:post:: /subscriptions

   Create a new subscription

   **Example request**:

   .. sourcecode:: http

      GET /subscriptions HTTP/1.1
      Host: example.com
      Content-Type: application/json

      {
          "feature_name": "switch_1",
          "event_type": "State",
          "callback": "http://my-server.local:8000/notify",
          "timeout": 3600
      }

   **Example response**:

   .. sourcecode:: http

      HTTP/1.1 200 OK
      Content-Type: text/javascript

   :<json string feature_name: The device to use
   :<json string event_type: The type of event to subscribe to
   :<json string callback: The callback to call when the event occurs
   :<json integer timeout: The timeout in seconds
   :status 200: no error
   :status 400: when form parameters are missing
