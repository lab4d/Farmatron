# Farmatron
NFT Hydroponic farm controller


Simple Arduino software to control remotely a small home hydroponic system

The software allows customizing watering timers, while also providing temperature and humidity feedback.

It also hosts a local website for monitoring and configuration, as well as features Thingspeak upload and MQTT data.
the website allows turning on and off a fan i use in the greenhouse but mostly for debugging reasons, i suggest to keep the fan on 24/7.

Variables to quickly disable features are available, EnableSerial, EnableWeb, Enable MQTT etc ..

I will try to upload a schematic and images but it is quite simple and i am sure if you can navigate the code you can figure that out too.

Disclaimer
The code sometimes has issues with the MQTT library and from time to time locks up and sends crazy values over MQTT for some reason.
The watering is never affected by it but its something i will have to fix soon.
