//Constants
const int  SERIAL_RATE = 115200;
const int  SERIAL_TIMEOUT = 5;
const byte readPin = 21;
const byte writePin = 16;


int    rampUpTimes[4096];
int    rampUpValues[4096];
int    rampUpLength;
int    rampDownTimes[4096];
int    rampDownValues[4096];
int    rampDownLength;
int    laser_intensity_time;
double microsPerWrite;
bool   writeValue;
bool   readValue;
int highramp = 4095;
int lowramp = 1024;
int diff = highramp-lowramp;


void setup() {
  analogWriteResolution(12);
  SerialUSB.begin(SERIAL_RATE);
  SerialUSB.setTimeout(SERIAL_TIMEOUT);
  pinMode(readPin, INPUT);
  pinMode(writePin, OUTPUT);
  pinMode(A21, OUTPUT);
  pinMode(A22, OUTPUT);

  //Default to keeping laser off
  readValue = LOW;
  writeValue = LOW;
  analogWrite(A21, 0);
  digitalWrite(writePin,LOW);

  //Default to an evenly spaced linear ramp taking 300us
  laser_intensity_time=300;
  rampUpLength = 60;
  rampDownLength = 60;
  microsPerWrite = 5.;

  for (int upIndex = 0; upIndex < rampUpLength; ++upIndex) {
    rampUpTimes[upIndex] = int(microsPerWrite) * upIndex;
    rampUpValues[upIndex] = lowramp + round(double(upIndex) / rampUpLength * diff);
  }

  for (int downIndex = 0; downIndex < rampDownLength; ++downIndex) {
    rampDownTimes[downIndex] = int(microsPerWrite)* downIndex;
    rampDownValues[downIndex] = highramp -round(double(downIndex) / rampDownLength * diff);
  }

  //while (!SerialUSB) {}
}

void loop() {
  if (SerialUSB.available() > 0) {
    String header = SerialUSB.readStringUntil('>');
    if (header == "i") {
      //Update the intensity waveforms
      updateWaveforms();
    }
    else if(header =="r"){
      //send a response to let python know this board hasn't stalled
      sendok();
    }
    else {
      //Assume we read junk and clear buffer
      while (SerialUSB.available() > 0) {
        SerialUSB.read();
      }
    }
  }
  bool readValue = digitalRead(readPin);
  if(readValue != writeValue){
    if (readValue == HIGH){
      //writeWaveform(rampDownValues,rampDownTimes,rampDownLength);
      writeWaveform(rampUpValues,rampUpTimes,rampUpLength);
    }
    else{
      //writeWaveform(rampUpValues,rampUpTimes,rampUpLength);
      writeWaveform(rampDownValues,rampDownTimes,rampDownLength);
    }
    writeValue = readValue;
    digitalWrite(writePin,writeValue);

  }
}

void updateWaveforms(){
  /*
   * Read in updated intensity profiles from python
   */
  laser_intensity_time = SerialUSB.readStringUntil('>').toInt();

  
  rampUpLength = max(round(laser_intensity_time/microsPerWrite),1);
  rampDownLength = max(round(laser_intensity_time/microsPerWrite),1);

  
  if (rampUpLength > diff) rampUpLength = diff;
  if (rampDownLength > diff) rampDownLength = diff;


  
  for (int count = 0; count < rampUpLength; ++count){
    rampUpTimes[count] = int(microsPerWrite) * count;
    rampUpValues[count] = lowramp + round(double(count) / rampUpLength * diff);
  }


  
  for (int count = 0; count < rampDownLength;++count){
    rampDownTimes[count]=int(microsPerWrite) * count;
    rampDownValues[count] = highramp - round(double(count) / rampDownLength * diff);
  }
}

void writeWaveform(int values[], int times[], int arrayLength){
  /*
   * Given an array of values to write and times at which to write them,
   * as well as how many values/times there are, write the waveform to 
   * A21.
   */

  unsigned long startTime = micros();
  for (int count = 0; count < arrayLength; ++count){
    while(micros()-startTime < times[count]){}
    analogWrite(A21,values[count]);
  }
}

void sendok() {
  /*
    Print ok to the USB Serial stream to signal that the Arduino isn't stalled.
  */
  SerialUSB.println("ok");

}

