/***************************************************************************
* File Name: serial_MAX31865.h
* Processor/Platform: Arduino Uno R3 (tested)
* Development Environment: Arduino 1.0.5
*
* Designed for use with with Playing With Fusion MAX31865 Resistance
* Temperature Device (RTD) breakout board: SEN-30201 (PT100 or PT1000)
*   ---> http://playingwithfusion.com/productview.php?pdid=25
*   ---> http://playingwithfusion.com/productview.php?pdid=26
*
* Copyright © 2014 Playing With Fusion, Inc.
* SOFTWARE LICENSE AGREEMENT: This code is released under the MIT License. 
* 
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
* **************************************************************************
* REVISION HISTORY:
* Author			Date		Comments
* J. Steinlage			2014Jan25	Original version
* 
* Playing With Fusion, Inc. invests time and resources developing open-source
* code. Please support Playing With Fusion and continued open-source 
* development by buying products from Playing With Fusion!
*
* **************************************************************************
* ADDITIONAL NOTES:
* This file configures then runs a program on an Arduino Uno to read a 
* MAX31865 RTD-to-digital converter breakout board and print results to
* a serial port. Communication is via SPI built-in library.
*    - Configure Arduino Uno
*    - Configure and read resistances and statuses from MAX31865 IC 
*      - Write config registers (MAX31865 starts up in a low-power state)
*      - RTD resistance register
*      - High and low status thresholds 
*      - Fault statuses
*    - Write formatted information to serial port
*  Circuit:
*    Arduino Uno   -->  SEN-30201
*    CS: pin 10    -->  CS
*    MOSI: pin 11  -->  SDI (must not be changed for hardware SPI)
*    MISO: pin 12  -->  SDO (must not be changed for hardware SPI)
*    SCK: pin 13   -->  SCLK (must not be changed for hardware SPI)
*    GND           -->  GND
*    5V            -->  Vin (supply with same voltage as Arduino I/O, 5V)
***************************************************************************/

// the sensor communicates using SPI, so include the hardware SPI library:
#include <SPI.h>
// include Playing With Fusion MAX31865 libraries
#include <PlayingWithFusion_MAX31865.h>              // core library
#include <PlayingWithFusion_MAX31865_STRUCT.h>       // struct library

//Sensor addresses:
const byte CONFIG_REG_W = 0x80; // Config register write addr
const byte CONFIG_VALUE = 0xC3; // Config register value (Vbias+Auto+FaultClear+50Hz, see pg 12 of datasheet)
const byte ADC_WORD_MSB = 0x01; // Addr of first byte of data (start reading at RTD MSB)
const byte ADC_FAULT_REG = 0x07; // Addr of the fault reg

// CS pin used for the connection with the sensor
// other connections are controlled by the SPI library)
const int CS_PIN = 10;

PWFusion_MAX31865_RTD rtd_ch0(CS_PIN);

void setup() {
  Serial.begin(9600);

  // setup for the the SPI library:
  SPI.begin();                        // begin SPI
  SPI.setDataMode(SPI_MODE3);         // MAX31865 is a Mode 3 device
  
  // initalize the chip select pin
  pinMode(CS_PIN, OUTPUT);
  rtd_ch0.MAX31865_config();
  
  // give the sensor time to set up
  delay(100);
}


void loop() 
{
  delay(500);                                   // 500ms delay... can be much faster
  

  static struct var_max31865 RTD_CH0;
  double tmp;
  
  RTD_CH0.RTD_type = 1;                         // un-comment for PT100 RTD
  // RTD_CH0.RTD_type = 2;                        // un-comment for PT1000 RTD
  
  struct var_max31865 *rtd_ptr;
  rtd_ptr = &RTD_CH0;
  
  rtd_ch0.MAX31865_full_read(rtd_ptr);          // Update MAX31855 readings 
  
  // Print information to serial port
  Serial.println("RTD Sensor 0:");              // Print RTD0 header
  
  if(0 == RTD_CH0.status)                       // no fault, print info to serial port
  {
    if(1 == RTD_CH0.RTD_type)                   // handle values for PT100
    {
      // calculate RTD resistance
      tmp = (double)RTD_CH0.rtd_res_raw * 400 / 32768;
      Serial.print("Rrtd = ");                  // print RTD resistance heading
      Serial.print(tmp);                        // print RTD resistance
    }
    else if(2 == RTD_CH0.RTD_type)              // handle values for PT1000
    {
      // calculate RTD resistance
      tmp = (double)RTD_CH0.rtd_res_raw * 4000 / 32768;
      Serial.print("Rrtd = ");                  // print RTD resistance heading
      Serial.print(tmp);                        // print RTD resistance
    }
    Serial.println(" ohm");
    // calculate RTD temperature (simple calc, +/- 2 deg C from -100C to 100C)
    // more accurate curve can be used outside that range
    tmp = ((double)RTD_CH0.rtd_res_raw / 32) - 256;
    Serial.print("Trtd = ");                    // print RTD temperature heading
    Serial.print(tmp);                          // print RTD resistance
    Serial.println(" deg C");                   // print RTD temperature heading
  }  // end of no-fault handling
  else 
  {
    Serial.print("RTD Fault, register: ");
    Serial.print(RTD_CH0.status);
    if(0x80 & RTD_CH0.status)
    {
      Serial.println("RTD High Threshold Met");  // RTD high threshold fault
    }
    else if(0x40 & RTD_CH0.status)
    {
      Serial.println("RTD Low Threshold Met");   // RTD low threshold fault
    }
    else if(0x20 & RTD_CH0.status)
    {
      Serial.println("REFin- > 0.85 x Vbias");   // REFin- > 0.85 x Vbias
    }
    else if(0x10 & RTD_CH0.status)
    {
      Serial.println("FORCE- open");             // REFin- < 0.85 x Vbias, FORCE- open
    }
    else if(0x08 & RTD_CH0.status)
    {
      Serial.println("FORCE- open");             // RTDin- < 0.85 x Vbias, FORCE- open
    }
    else if(0x04 & RTD_CH0.status)
    {
      Serial.println("Over/Under voltage fault");  // overvoltage/undervoltage fault
    }
    else
    {
      Serial.println("Unknown fault, check connection"); // print RTD temperature heading
    }
  }  // end of fault handling
}

