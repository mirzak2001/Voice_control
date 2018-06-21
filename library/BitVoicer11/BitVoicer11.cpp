/*
  BitVoicer.cpp - Implements WIFI_SERIAL communication with the BitVoicer.
  Created by BitSophia Software Ltd., Dcember 03, 2012.
  Realeased into public domain.
*/

#include "Arduino.h"
#include "wiring_private.h"
#include "BitVoicer11.h"
#include <SoftwareSerial.h>
extern SoftwareSerial mySerial;
#define WIFI_SERIAL    mySerial

//Class constructor
//Receives an integer indicating the WIFI_SERIAL port to be used
BitVoicerSerial::BitVoicerSerial()
{
  //Sets all public variables to NULL, engine status to stopped
  //and the analog voltage reference
  byteData = NULL;
  charData = NULL;
  intData = NULL;
  strData= "";
  cmdData = NULL;
  engineRunning = false;
  _bvAnalogReference = BV_DEFAULT;
}

//Retrieves data from WIFI_SERIAL buffer and stores it in the
//correct public variable.
//Returns the received data type (byte) or ZERO if corrupted
//data is received or if no data is available
byte BitVoicerSerial::getData()
{
  //Creates internal variables
  byte dataType = 0;
  byte dataLength = 0;
  int bytesAvailable = 0;

  //Gives Arduino a time to finish receiving WIFI_SERIAL
  //data. This delay can be reduced at faster reading
  //speeds or when receiving small datagrams
  if (WIFI_SERIAL.available() > 0)
  {
    delay(100);
    bytesAvailable = WIFI_SERIAL.available();
  }

  //If the received data is smaller than the minimum datagram
  //size or the first byte is not the start of transmission
  //mark, NO operation is performed
  if (bytesAvailable >= 5 && WIFI_SERIAL.peek() == 1)
  {
    //Reads the start of transmission mark
    WIFI_SERIAL.read();
    //Reads and stores the data type
    dataType = WIFI_SERIAL.read();
    //Reads and stores the data length
    dataLength = WIFI_SERIAL.read();

    //Reads and stores the data for the expected length
    for (int i = 0; i < dataLength; i++)
      _data[i] = WIFI_SERIAL.read();

    //If the end os transmission mark is not found,
    //returns data type to zero. All data will be
    //disregarded
    if (WIFI_SERIAL.read() != 4)
    {
      dataType = 0;
    }
    else
    {
      //Sets the data in the temporary buffer to the correct
      //public variable
      setData(dataType, dataLength);
      //Returns the retrived data type
      return dataType;
    }
  }

  //Clears the WIFI_SERIAL buffer until the start of transmission
  //mark is found and there is 5 bytes available
  for (int i = 0; i < bytesAvailable; i++)
  {
    if (WIFI_SERIAL.peek() == 1 && WIFI_SERIAL.available() < 5)
      WIFI_SERIAL.read();
    else if (WIFI_SERIAL.peek() == 1 && WIFI_SERIAL.available() >= 5)
      break;
    else
      WIFI_SERIAL.read();
  }

  //returns ZERO because:
  //- No data was available or
  //- Corrupted data has been received or
  //- The received datagram is invalid
  return dataType;
}

//Retrieves data from _data and sets the
//respective public variables
void BitVoicerSerial::setData(byte dataType, int dataLength)
{
  //Validates passed variables
  if (dataType == 0 || dataLength <= 0)
  {
    byteData = NULL;
    charData = NULL;
    intData = NULL;
    strData= "";
    cmdData = NULL;
    return;
  }

  //Selects the correct procedure for each data type,
  //retrieves the data from internal buffer and
  //sets it in the correct variable
  switch (dataType)
  {
    //Case byte
    case BV_BYTE:
      //Sets byte public variable to the first byte in the
      //internal buffer
      byteData = _data[0];
      charData = NULL;
      intData = NULL;
      strData= "";
      cmdData = NULL;
      break;
    //Case char
    case BV_CHAR:
      byteData = NULL;
      //Sets char public variable to the converted first byte
      //in the internal buffer
      charData = char(_data[0]);
      intData = NULL;
      strData= "";
      cmdData = NULL;
      break;
    //Case int
    case BV_INT:
      byteData = NULL;
      charData = NULL;
      //Converts 2 bytes from internal buffer into
      //Arduino 16 bits int and sets int public variable
      intData = _data[0] | (_data[1] << 8);
      strData= "";
      cmdData = NULL;
      break;
    //Case String
    case BV_STR:
      byteData = NULL;
      charData = NULL;
      intData = NULL;
      strData = "";

      //Loops the internal buffer and concatenates the
      //data into the string public variable
      for (int i = 0; i < dataLength; i++)
        strData += char(_data[i]);

      cmdData = NULL;
      break;
    //Case command
    case BV_COMMAND:
      byteData = NULL;
      charData = NULL;
      intData = NULL;
      strData = "";
      //Sets command public variable to the first byte in the
      //internal buffer
      cmdData = _data[0];
      break;
    //Case status
    case BV_STATUS:
      //Checks the received status data and sets the engine
      //status public variable
      engineRunning = _data[0];
  }

}

//Signals the BitVoicer's recognition engine to start or stop
//processing audio streams
void BitVoicerSerial::startStopListening()
{
  //The start/stop signal
  byte signal[] = {255, 255, 255, 255, 0, 0, 0, 0};
  //Sends the signal to the WIFI_SERIAL port
  for (int i = 0; i < 8; i++)
    WIFI_SERIAL.write(signal[i]);
}

//Fast reads the analog input port and sends the result
//to BitVoicer. It receives a delay value that can be used
//to equalize the stream rate
void BitVoicerSerial::processAudio(unsigned int wait)
{
  //Delays the execution for the passed time (microseconds)
  if (wait > 0)
    delayMicroseconds(wait);

  //Starts the ADC conversion
  sbi(ADCSRA, ADSC);
  //Whaits for the ADC convertion to finish
  while (bit_is_set(ADCSRA, ADSC));
  //Sends the 8 bit result to BitVoicer
  WIFI_SERIAL.write(ADCH);
}

//Changes the analog voltage reference on the board
//Do NOT use arduino constants when calling this function!!!
//You may end up burning your board
void BitVoicerSerial::setAnalogReference(uint8_t mode)
{
  _bvAnalogReference = mode;
}

//Sets up the microcontroller to perform faster analog
//conversions on the specified pin
void BitVoicerSerial::setAudioInput(uint8_t pin)
{
  //Check if the pin value is in the accepted range
  if (pin > 5)
    return;

  //Sets the analog reference (high two bits of ADMUX),
  //selects the pin (low 4 bits) and sets ADLAR to left-adjust
  //the reslt (writes 1 to ADLAR)
  ADMUX = ((_bvAnalogReference << 5) | (pin & 0x07));
  //Sets the ADC clock to a faster clock
  //Since an 8 bit resolution is sufficient, it does not affect
  //the ADC accuracy
  sbi(ADCSRA, ADPS2);
  sbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
}

//Returns the ADC and ADMUX to their original settings
void BitVoicerSerial::undoAudioInput()
{
  ADMUX = 0x00;
  sbi(ADCSRA, ADPS2);
  sbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);
}

//BitVoicerSerial.sendToBV()
//Aceita byte, char, int, string
//////////////////////////////////////////////
//Wraps one byte in the BitVoicer Protocol and
//sends it to BitVoicer
void BitVoicerSerial::sendToBV(uint8_t b)
{
  WIFI_SERIAL.write(1);
  WIFI_SERIAL.write(BV_BYTE);
  WIFI_SERIAL.write(1);
  WIFI_SERIAL.write(b);
  WIFI_SERIAL.write(4);
}

//Wraps one char in the BitVoicer Protocol and
//sends it to BitVoicer
void BitVoicerSerial::sendToBV(char c)
{
  WIFI_SERIAL.write(1);
  WIFI_SERIAL.write(BV_CHAR);
  WIFI_SERIAL.write(1);
  WIFI_SERIAL.write((byte)c);
  WIFI_SERIAL.write(4);
}

//Wraps one int in the BitVoicer Protocol and
//sends it to BitVoicer
void BitVoicerSerial::sendToBV(int n)
{
  WIFI_SERIAL.write(1);
  WIFI_SERIAL.write(BV_INT);
  WIFI_SERIAL.write(2);
  WIFI_SERIAL.write((byte)(n & 0XFF));
  WIFI_SERIAL.write((byte)((n >> 8) & 0XFF));
  WIFI_SERIAL.write(4);
}

//Wraps a String object in the BitVoicer Protocol and
//sends it to BitVoicer
void BitVoicerSerial::sendToBV(String s)
{
  WIFI_SERIAL.write(1);
  WIFI_SERIAL.write(BV_STR);
  WIFI_SERIAL.write(s.length());

  for (int i = 0; i < s.length(); i++)
    WIFI_SERIAL.write((byte)s.charAt(i));

  WIFI_SERIAL.write(4);
}
