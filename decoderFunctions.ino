/*
 * decoderFunctions.ino was taken from this site:
 * https://www.digitaltown.co.uk/project6ESP32DCCAccDecodert.php
 *
 * copyright to digitaltown!
 *
 * Use script at your own risk, no responsibility taken for your ESP32 blowing up, house burning down, meteor strike or plague.
 * Use it at your own risk.
 *
 * DO NOT CONNECT DIRECT TO TRACK
 *
 * Use the circuit found on https://www.digitaltown.co.uk/79DCCDecoderCircuit.php
 *
 * This script is set up for the Interrupt Jumper on step 11 to be connected to GPIO4 on the ESP32 Dev Module
 */
void processDCC() {
  byte thisByte;
  int interruptTime;
  if (timingSync < 1) {//check if the interrupt is working correctly does it need reversing?
    if(timingSyncCounter < 200){//check 200 bits...won't take long
      //Serial.println(timingSyncCounter);
      if (ISRRISING > 0) {
        if(timingSyncCounter > 9){//allow a few cycles for everything to settle down
          interruptTime = ISRRISETime - ISRLastRISETime;//time between interrupts
          if(interruptTime < 125 || interruptTime > 200){//check the interrupt is within DCC parameters
            
            //Serial.println(interruptTime);    
          }else{
            Serial.print("F: ");
            Serial.println(interruptTime);
            timingSyncCounter = 0;//ready to start again
            detachInterrupt(digitalPinToInterrupt(dccpin));
            if(timeRiseFall < 1){
              timeRiseFall = 1;
              attachInterrupt(digitalPinToInterrupt(dccpin),pinstate, FALLING);
              Serial.println("Switch to FALLING");    
            }else{
              timeRiseFall = 0;
              attachInterrupt(digitalPinToInterrupt(dccpin),pinstate, RISING);
              Serial.println("Switch to RISING");  
            }
          }
        }
        timingSyncCounter++;
        ISRLastRISETime = ISRRISETime;
        ISRRISING = 0;
           
      }
    }else{//greater than 200 succesful so get the main system running
      timingSync= 1;
      timingSyncCounter = 0;
      Serial.println("Sync Complete...time for work");  
    }
  } else {
    if (ISRRISING > 0) {
      if ((ISRRISETime - ISRLastRISETime) > onebittime) {
        thisByte = 0;
      } else {
        thisByte = 1;
      }
      ISRRISING = 0;
      ISRLastRISETime = ISRRISETime;
      if (Preamble < 1) { //hasn't finished preamble
        if (thisByte == 1) {
          PreambleCounter++;
        } else {
          PreambleCounter = 0;
        }
        if (PreambleCounter > 9) { //NMRA say a decoder must receive aminimum of 10 "1" bits before accepting a packet
          Preamble = 1;//pramble condition met
          PacketStart = 0;
          PreambleCounter = 0;
        }
      } else { //preamble has been completed, wait for packet start
        if (PacketStart < 1) {
          if (thisByte == 0) {
            PacketStart = 1;
            bitcounter = 1;
          }
        } else { //we have a preamble and a packet start so now get the rest of the data ready for processing
          packetdata[bitcounter] = thisByte;
          bitcounter++;
          if (bitcounter > 27) { //should now have the whole packet in array
            Preamble = 0;
            PreambleCounter = 0;
            //uncomment if you want to see the raw data
            /*
              for(int q = 0;q<28;q++){
              Serial.print(packetdata[q]);
              }
              Serial.println(" ");
            */
            processpacket();
          }
        }
      }
    }
  }
}

void AccDecoder(byte AddressByte, byte InstructionByte, byte ErrorByte) {
  byte index;
  byte dir;
  int AccAddr;
  int BoardAddr;

  if (packetdata[27] == 1) { //basic packet
    dir = bitRead(InstructionByte, 0);
    index = bitRead(InstructionByte, 1);
    if (bitRead(InstructionByte, 2) > 0) {
      bitSet(index, 1);
    }

    BoardAddr = AddressByte - 0b10000000;
    //now get the weird address system from instruction byte
    if (bitRead(InstructionByte, 4) < 1) {
      bitSet(BoardAddr, 6);
    }
    if (bitRead(InstructionByte, 5) < 1) {
      bitSet(BoardAddr, 7);
    }
    if (bitRead(InstructionByte, 6) < 1) {
      bitSet(BoardAddr, 8);
    }
    AccAddr = ((BoardAddr - 1) * 4) + index + 1;
    /*
      Serial.println(index);
      Serial.println(dir);
      Serial.println(AccAddr);
      Serial.println(BoardAddr);
    */
    ControlAccDecoder(index, dir, AccAddr, BoardAddr);
  } else {
    Serial.print("Ext pckt format not supported yet");
  }
}

void processpacket() {
  byte  AddressByte = 0;
  byte  InstructionByte = 0;
  byte  ErrorByte = 0;
  byte errorTest;
  for (int q = 0; q < 8; q++) {
    if (packetdata[1 + q] > 0) {
      bitSet(AddressByte, 7 - q);
    } else {
      bitClear(AddressByte, 7 - q);
    }

    if (packetdata[10 + q] > 0) {
      bitSet(InstructionByte, 7 - q);
    } else {
      bitClear(InstructionByte, 7 - q);
    }

    if (packetdata[19 + q] > 0) {
      bitSet(ErrorByte, 7 - q);
    } else {
      bitClear(ErrorByte, 7 - q);
    }

  }
  //certain Addressess are reserved for the system so not shown to reduce load
  if (AddressByte > 0 && AddressByte != 0b11111110 && AddressByte != 0b11111111 && bitRead(AddressByte, 7) == 1 && bitRead(AddressByte, 6) == 0) {
    errorTest = AddressByte ^ InstructionByte;
    //  Serial.print("Add: ");
    //  Serial.print(AddressByte,BIN);

    //  Serial.print(" Inst: ");
    //  Serial.print(InstructionByte,BIN);

    //  Serial.print(" Err: ");
    //  Serial.println(ErrorByte,BIN);
    
     
    //  Serial.print("test2: ");
    //  Serial.println(errorTest,BIN);
    if(errorTest == ErrorByte){ //only send command if error byte checks out
      AccDecoder(AddressByte, InstructionByte, ErrorByte);
    }
  }
}
