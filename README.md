# Steuerungssoftware_Gasloeslichkeitspruefstand
  
Control Software for a gas solubiltiy testrig. Control Software written in C++ runs on a microcontroller, such as the ATmega2560 on the Arduino MEGA board. The User Interface is realized with a Java based Ecplipse IDE. IN the test rig this part of the software runs on the unix plattform of a raspberry pi.

## Getting Started
These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.
  

### Prerequisites and Installing
Visual Studio Code with Plattform IO Extension - https://code.visualstudio.com/download , https://platformio.org/
Eclipse IDE on the raspberry pi system - https://www.eclipse.org/downloads/
Node package manager - https://www.npmjs.com/
  

## Running the Model/Code
First the control software must be compiled and flashed onto the ATmega2560 microcontroller using Plattform IO. The control software is located under the subfolder: PlatformIO/Projects/Control_Software:

The User Interface can be Run on a Unix machine, such as the used raspberry pi by navigating into the subfolder: Steuersoftware_GUI and run the java script code using the Node package manager with: npm Started


## Contributing and Support
  
Contribution can be realized by contacting the Instituta of Automotive Technology at TUM
  

## Versioning

V1.0 Final Running Version
  

## Authors
Ahmet Bulut - Initial work and Development of Code Architecture
Andreas Eisele - Development, Bug Fixes and HArdware Testing
Simon Sagmeister - Development of Controller and overall Software Concept
  
## License
This project is licensed under the LGPL License - see the LICENSE.md file for details
 
 
## Sources
Ahmet Bulut: "Development of a Control Software and Set Up of a Mechatronical Test Rig for Gas Solubility Measurements" , Bachelors Thesis, TUM, 2020