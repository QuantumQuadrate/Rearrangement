//Constants
const int xatoms = 11;                   //number of columns in the lattice
const int yatoms = 11;                   //number of rows in the lattice
const byte executeInstructionsPin = 11;  //Pin which triggers board to write stored waveforms
const byte switchPin = 15;               //Low = VCO signal, High = external signal
const byte intensityWritePin = 4;       //Pin used to send intenisty requests to other board
const byte intensityReadPin = 9;        //Pin used to read current intensity from other board

//Rearrangement Variables
bool previousstate;                      //Tracks previous loop state of executeInstructionsPin
double freqjump;                         //Resulting jump in frequency between successive writes
int xjump;                               //difference in values we write to A22 port on successive writes
int yjump;                               //difference in values we write to the A21 port on successive writes
int xcoords[yatoms][xatoms];             //x value to write for the lattice sites
int ycoords[yatoms][xatoms];             //y value to write for the lattice sites
double xfreqs[yatoms][xatoms];           //x-axes frequency for the lattice sites
double yfreqs[yatoms][xatoms];           //y-axes frequency for the lattice sites
int nummoves;                            //number of moves being executed
int coordsformoves[xatoms * yatoms * 4]; //buffer for lattice coordinates for the moves 
int vcodelay;                            //us to wait for vco to settle into position
int movedelay;                           //us to wait inbetween move writes
double xslope = 74.937;                  //Used to determine jump size
double yslope = 75.360;                  //Used to determine jump size
double xcalfreqs[] = {128.89,130.45,132.04,133.645,135.28,136.915,138.55,140.23,141.88,143.575,145.27,146.98,148.705,150.43,152.155,153.865,155.575,157.345,159.115,160.855,162.64,164.335,166.06,167.785,169.495,171.175,172.885,174.61,176.245,177.955,179.71,181.465,183.07};
double calibvals[] = {0,128,256,384,512,640,768,896,1024,1152,1280,1408,1536,1664,1792,1920,2048,2176,2304,2432,2560,2688,2816,2944,3072,3200,3328,3456,3584,3712,3840,3968,4095};
double ycalfreqs[] = {127.39,128.875,130.45,132.07,133.66,135.295,136.9,138.535,140.155,141.835,143.515,145.255,146.965,148.675,150.385,152.095,153.805,155.515,157.27,159.01,160.78,162.535,164.23,165.955,167.65,169.33,170.98,172.69,174.43,176.14,177.865,179.575,181.285};
int caliblength = 33;
void setup() {
  //Open Serial Connection
  SerialUSB.begin(115200);
  SerialUSB.setTimeout(5);
  //while (!SerialUSB) {} 
  
  //Resolve Pin modes and resolutions
  pinMode(executeInstructionsPin, INPUT);
  pinMode(intensityReadPin,INPUT);
  pinMode(switchPin, OUTPUT);
  pinMode(intensityWritePin, OUTPUT);
  analogWriteResolution(12);
  pinMode(A21, OUTPUT);
  pinMode(A22, OUTPUT);

  //Set box defaults
  switchAOMToExternal();
  forceLaserOff();
  
  //Set default rearrangement parameters
  vcodelay = 10;          
  movedelay = 100;        
  freqjump = 0.01;                      
  previousstate = LOW;
  xjump = max(1, floor(freqjump * xslope));
  yjump = max(1, floor(freqjump * yslope));
  nummoves = 0;
  for (int i = 0; i < xatoms * yatoms * 4; i++) {
    coordsformoves[i] = 0;
  }
  for (int j = 0; j < yatoms; j++) {
    for (int i = 0; i < xatoms; i++) {
      xcoords[j][i] = 0;
      ycoords[j][i] = 0;
      xfreqs[j][i]  = 0.;
      yfreqs[j][i]  = 0.;
    }
  }
  
}

void loop() {
  if (SerialUSB.available() > 0) {
    String header = SerialUSB.readStringUntil('>');
    //At this point header is a letter informing us of the content
    if (header == "f") {
      //f means we have been asked to report on what the microcontroller thinks all the variables are.
      sendVariablesToPython();
    }
    else if (header == "u") {
      //u is for updating the moves that need to be written the next time we get a trigger
      readCurrentInstructions();
    }
    else if (header == "r") {
      //r is for checking in to see if the Arduino has stalled
      sendok();
    }
    else if (header == "c") {
      //c is for setting frequencies and movement profiles
      readCalibrationData();
    }
    else {
      //assume we read garbage noise and clear the buffer
      while (SerialUSB.available() > 0) {
        SerialUSB.read();
      }
    }
  }
  bool currentstate = digitalRead(executeInstructionsPin);
  if (currentstate == HIGH and previousstate == LOW) { 
    //Execute instructions on a rising pulse
    executeCurrentInstructions();
    //Set nummoves = 0 to prevent an accidental repeat of execution
    nummoves = 0;
    //Now that the instruction set is complete, switch AOM and laser power control away from Teensy
    delayMicroseconds(vcodelay);
    forceLaserOff();
    switchAOMToExternal();
  }
  previousstate = currentstate;
}

int frequencyToIntX(double freq) {
  /*
     Convert a frequency to a DAC output integer that will provide that frequency on lattice x-axis.
     If outside the frequency range of the AO, snap to the nearest end.
  */
  if (freq < 130) {
    return frequencyToIntX(130.);
  }
  else if (freq > 180) {
    return frequencyToIntX(180.);
  }
   for(int i = 0; i < caliblength;++i){
    if (freq == xcalfreqs[i]){
      return calibvals[i];
      }
    if (freq < xcalfreqs[i]){ //Interpolate between the surrounding points we have data on.
      return calibvals[i-1] + round( (calibvals[i]-calibvals[i-1])* (freq-xcalfreqs[i-1])/(xcalfreqs[i]-xcalfreqs[i-1]));
    }
  }
  return 0;
}

int frequencyToIntY(double freq) {
  /*
     Convert a frequency to a DAC output integer that will provide that frequency on lattice y-axis.
     If outside the frequency range of the AO, snap to the nearest end.
  */
  if (freq < 130) {
    return frequencyToIntY(130.);
  }
  else if (freq > 180) {
    return frequencyToIntY(180.);
  }
  for(int i = 0; i < caliblength;++i){
    if (freq == ycalfreqs[i]){
      return calibvals[i];
      }
    if (freq < ycalfreqs[i]){ //Interpolate between the surrounding points we have data on.
      return calibvals[i-1] + round( (calibvals[i]-calibvals[i-1])* (freq-ycalfreqs[i-1])/(ycalfreqs[i]-ycalfreqs[i-1]));
    }
  }
  return 0;
}

void sendVariablesToPython() {
  /*
     Send a string containing all rearrangement variables to python.
  */
  String response = String(movedelay) + ">" + String(freqjump) + ">";
  for (int j = 0; j < yatoms; j++) {
    for (int i = 0; i < xatoms; i++) {
      response = response + String(xfreqs[j][i]) + ">" + String(yfreqs[j][i]) + ">";
    }
  }
  SerialUSB.println(response);
}

void readCurrentInstructions() {
  /*
     Store the lattice coordinates for the next set of instructions to apply if we recieve a trigger
  */
  nummoves = SerialUSB.readStringUntil('>').toInt();
  //Make sure we don't overflow the buffer to access bad memory
  if (nummoves > xatoms*yatoms*4) nummoves = xatoms*yatoms*4;
  //Moves are formatted as y1>x1>y2>x2>
  for (int i = 0; i < nummoves; i++) {
    coordsformoves[4 * i] = SerialUSB.readStringUntil('>').toInt();
    coordsformoves[4 * i + 1] = SerialUSB.readStringUntil('>').toInt();
    coordsformoves[4 * i + 2] = SerialUSB.readStringUntil('>').toInt();
    coordsformoves[4 * i + 3] = SerialUSB.readStringUntil('>').toInt();
  }
}

void sendok() {
  /*
    Print ok to the USB Serial stream to signal that the Arduino isn't stalled.
  */
  SerialUSB.println("ok");

}

void readCalibrationData() {
  /*
     Update variables that describe how movement occurs.
     These include the frequencies of each site,
     delay times for various parts of the waveforms,
     and the amount by which we want to allow frequencies to change between succesive writes.
  */
  //Details for the actual movement process
  movedelay = SerialUSB.readStringUntil('>').toInt();
  freqjump = SerialUSB.readStringUntil('>').toFloat();

  //Fill in our tables of frequency and DAC Output coordinates for each atom
  for (int j = 0; j < yatoms; j++) {
    for (int i = 0; i < xatoms; i++) {
      xfreqs[j][i] = SerialUSB.readStringUntil('>').toFloat();
      yfreqs[j][i] = SerialUSB.readStringUntil('>').toFloat();
      xcoords[j][i] = frequencyToIntX(xfreqs[j][i]);
      ycoords[j][i] = frequencyToIntY(yfreqs[j][i]);
    }
  }
  //If the requested frequency jump is too precise for our resolution, default to max resolution
  xjump = max(1, floor(xslope * freqjump));
  yjump = max(1, floor(yslope * freqjump));
}

void executeCurrentInstructions() {
  /*
     Write the waveforms to the VCO's to perform rearrangement.
  */
  //Ensure laser power is zero before taking action
  forceLaserOff();
  switchAOMToVCO();
  for (int i = 0; i < nummoves; i++) {
    //Lattice Coordinates of start and endpoints of move
    int ystart = coordsformoves[4 * i];
    int xstart = coordsformoves[4 * i + 1];
    int yend = coordsformoves[4 * i + 2];
    int xend = coordsformoves[4 * i + 3];
    int ycorn = yend;
    int xcorn = xstart;

    //Get the values to write to the DACs for the beginning and end of the move
    int DACxstart = xcoords[ystart][xstart];
    int DACystart = ycoords[ystart][xstart];
    int DACxend   = xcoords[yend][xend];
    int DACyend   = ycoords[yend][xend];
    int DACxcorn  = xcoords[ycorn][xcorn];
    int DACycorn  = ycoords[ycorn][xcorn];

    //This is a corner move meant to drag an atom into a corner and eject it.
    if(yend<0 or xend<0){
      if (ystart>0 and xstart>0){
        //Interpolate end coordinate between current site and the site to upper left
        DACxcorn =  (xcoords[ystart][xstart]+xcoords[ystart-1][xstart-1])/2;
        DACycorn  = (ycoords[ystart][xstart]+ycoords[ystart-1][xstart-1])/2;
        }
      else if (ystart>0){
        //Interpolate end coordinate between current site and the site to upper right
        DACxcorn =  (xcoords[ystart][xstart]+xcoords[ystart-1][xstart+1])/2;
        DACycorn  = (ycoords[ystart][xstart]+ycoords[ystart-1][xstart+1])/2;
        }  
      else{
        //Interpolate end coordinate between current site and the site to lower left
        DACxcorn =  (xcoords[ystart][xstart]+xcoords[ystart+1][xstart-1])/2;
        DACycorn  = (ycoords[ystart][xstart]+ycoords[ystart+1][xstart-1])/2;
        }
        DACxend = DACxcorn;
        DACyend = DACycorn;
      }

    //Set the AOM to point at the starting location of the move
    analogWrite(A21, DACxstart);
    analogWrite(A22, DACystart);
    delayMicroseconds(vcodelay);

    //Ensure Laser is on before we start changing frequencies
    allowLaserOn();
    //Resolve discrepencies in the lattice Y-axis first
    if (ystart != yend) {
      moveAtom(DACystart, DACxstart, DACycorn, DACxcorn);
    }

    //Resolve discrepencies in the lattice X-axis second
    if (xstart != xend) {
      moveAtom(DACycorn, DACxcorn, DACyend, DACxend);
    }

    //Ensure laser is off before we go on to the next move
    forceLaserOff();
  }
}

void allowLaserOn() {
  //Send signal to Intensity board to stop attenuating the VCO signal.
  adjustLaser(HIGH);
}

void forceLaserOff() {
  //Send signal to Intensity board to fully attenuate the VCO signal.
  adjustLaser(LOW);
}

void adjustLaser(bool desiredIntensity){
  //Tell other board our desired intensity.
  //Wait until other board tells this board the actual intensity agrees with the desired.
  digitalWrite(intensityWritePin, desiredIntensity);
  bool intensityValue;
  do {
    intensityValue = digitalRead(intensityReadPin);
  } while (intensityValue != desiredIntensity) ;
}

void switchAOMToVCO() {
  //Set the box to forward the VCO signals to the AOM
  digitalWrite(switchPin, LOW);
}

void switchAOMToExternal() {
  //Set the box to forward the External signals to the AOM
  digitalWrite(switchPin, HIGH);
}

void moveAtom(int ystart, int xstart, int yend, int xend) {
  /*
     Given DAC-Value coordinates for the start and end location of an atom,
     update the values wrriten to the DACs according to the desired velocity profile.
  */
  //Find out whether we must iterate up or down in each dimension
  int xiter = xend - xstart > 0 ? xjump : -xjump;
  int yiter = yend - ystart > 0 ? yjump : -yjump;
  unsigned long micro = micros();

  //Continue moving this atom until we are close enough to the destination
  while (abs(xend - xstart) >= xjump && abs(yend - ystart) >= yjump) {
    if (xiter * (xiter + 2 * (xstart - xend)) <= yiter * (yiter + 2 * (ystart - yend))) {
      //Changing x value puts us closer to final location than changing y
      xstart += xiter;
      while ( abs(micros() - micro) < movedelay) {} //Wait until our specified time between writes has passed
      analogWrite(A21, xstart);
    }
    else {
      //Changing y value puts us closer to final location than changing x
      ystart += yiter;
      while ( abs(micros() - micro) < movedelay) {} //Wait until our specified time between writes has passed
      analogWrite(A22, ystart);
    }
    micro = micros();
  }
  //Snap to the "true" endpoint and allow the atom to settle in the end location
  analogWrite(A22, yend);
  delayMicroseconds(movedelay);
  analogWrite(A21, xend);
  delayMicroseconds(vcodelay);
}


