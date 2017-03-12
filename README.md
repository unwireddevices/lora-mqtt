# lora-mqtt
Translator between binary LoRa application layer and MQTT broker

*lora-mqtt* is used to provide an interface between LoRa star network built on Unwired Devices's modules and network layer and high-level protocols and applications.

*lora-mqtt* works with Unwired Range LoRa modem connected to UART port (/dev/ttyATH0 by default) with *lora-star-uni-gate* firmware, and connects with Mosquitto MQTT broker. All recognized messages from the LoRa network (sensor data, join requests, etc.) are published as MQTT messages, and vice versa, MQTT messages published by external source are translated to commands specific for LoRa motes.