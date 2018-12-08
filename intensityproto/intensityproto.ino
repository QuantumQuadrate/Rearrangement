const int  SERIAL_RATE = 115200;
const int  SERIAL_TIMEOUT = 5;
const byte readPin = 21;
const byte writePin = 16;
String input;


int readinInt(){
  while(true){
    if(SerialUSB.available()>0){
      input = SerialUSB.readStringUntil('>');
      if (SerialUSB.available()>0){
        //SerialUSB.println("readinInt thinks there is more to be seen.");
        }
      return input.toInt();
    }
  }
}

void setup() {
  // put your setup code here, to run once:
   analogWriteResolution(12);
  SerialUSB.begin(SERIAL_RATE);
  SerialUSB.setTimeout(SERIAL_TIMEOUT);
  pinMode(readPin, INPUT);
  pinMode(writePin, OUTPUT);
  pinMode(A21, OUTPUT);
  pinMode(A22, OUTPUT);
  analogWrite(A21,4095);
  analogWrite(A22,4095);

}

void loop() {
  // put your main code here, to run repeatedly:
  switch( readinInt()){
    case 21:
    {
      int val = readinInt();
      analogWrite(A21,val);
      break;
    }
    case 22:
    {
      int val = readinInt();
      analogWrite(A22,val);
      break;
    }
}
}


