const int SERIAL_RATE = 115200;
const int SERIAL_TIMEOUT=5;
const byte executeInstructionsPin = 11;  //Pin which triggers board to write stored waveforms
const byte switchPin = 15;               //Low = VCO signal, High = external signal
const byte intensityWritePin = 4;       //Pin used to send intenisty requests to other board
const byte intensityReadPin = 9;        //Pin used to read current intensity from other board
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
  pinMode(executeInstructionsPin, INPUT);
  pinMode(intensityReadPin,INPUT);
  pinMode(switchPin, OUTPUT);
  pinMode(intensityWritePin, OUTPUT);
  analogWriteResolution(12);
  pinMode(A21, OUTPUT);
  pinMode(A22, OUTPUT);
  analogWrite(A21,500);
  analogWrite(A22,500);
  digitalWrite(switchPin,LOW);

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

