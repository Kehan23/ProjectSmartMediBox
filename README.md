# Project Smart Medi Box


The Smart Medi Box is an innovative device designed to help users manage their medication schedule effectively. Initially, users input their time zone and set alarms to receive timely reminders for taking their medicine. Additionally, the Medi Box is equipped with Light Dependent Resistors (LDRs) to protect medications from excessive light exposure.

## Key Features

- **Timely Reminders:** The device alerts users with alarms when it’s time to take their medicine.
- **Environmental Monitoring:** It continuously monitors the temperature and humidity within the Medi Box, notifying the user with visual and audio alerts if conditions fall outside the desired range.
- **Light Control:** A motorized curtain controls the amount of light entering the Medi Box, ensuring optimal storage conditions for the medicine.

## Project Phases

This project was completed in two phases. In the first phase, we developed and simulated a Medi Box that not only reminds users to take their medicine on time but also monitors the internal humidity and temperature, alerting the user if these conditions deviate from the optimal range. The simulation and code for this phase, titled `InitialCode.ino`, can be found above.
 The Wokwi link : https://wokwi.com/projects/389306992640954369

![Screenshot 2024-05-21 200821](https://github.com/Kehan23/ProjectSmartMediBox/blob/main/Images/FirstWokwi.png)

## Second Phase Enhancements

In the second phase of the project, several advanced features were incorporated to enhance the functionality of the Smart Medi Box:

- **Light Control:** A motorized curtain was implemented to regulate the light entering the Medi Box, ensuring optimal conditions for the stored medications.
- **Real-Time Monitoring Dashboard:** Node-RED was used to create a dynamic dashboard for real-time monitoring of the Medi Box’s environment.

Data communication between the ESP32 board and Node-RED was established using the MQTT protocol, facilitating efficient and reliable data exchange.

The final circuit design can be viewed on Wokwi, and the complete code for this phase is available in `FinalCode.ino`.

Wokwi link : https://wokwi.com/projects/397658126054951937

![Screenshot 2024-05-21 200716](https://github.com/Kehan23/ProjectSmartMediBox/blob/main/Images/FinalWokwi.png)
