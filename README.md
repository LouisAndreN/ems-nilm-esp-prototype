# Non-Intrusive Load Monitoring System & Energy Monitoring

The goal of this project is to create a monitoring system for home to track energy consumption of home devices such as frdge, induction heater, air conditionner, computers, charging smartphones, lights, ..., to detect anomalies if possible to anticipate maintenance, predict consumption of the household, anticipate devices usage time to predict device usage patterns to avoid tripping the circuit breaker.

This system is deployed in a japanese residence but can also be adapted for industrial fields, in manufacture or other urban buildings.

# Used material

- SCT-013 5A/1V (for first tests on power strip) and SCT-013 30A/1V
- ADS1115
- Adafruit Jack plug TRRS
- ESP32 S3 R16N8 (or ESP32-WROOM-32E)
- Set of resistors
- Set of capacitors
- Male plug and female plug JP type
- AWG14 cables
- Proster multimeter

# Future features

Some devices can be tracked but because of the relative low precision of the ADS1115, the system needs to be improved with an audio card 48kHz to get more harmonics to detect specific devices such as induction heater.
