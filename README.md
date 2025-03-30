# ESP32 DCC Accessory Decoder

With this arduino code, I can control DCC-EX accessories using my
Marklin Mobile Station 3. This is done by DCC signal to Arduino pcb, 
letting the Arduino to read the DCC Accessory signal and converting
this to DCC-EX command and sending this command by WiFi to the DCC-EX 
CommandStation. 

The code is specific for ESP32 cpu type and the DCC signal code is
taken from: https://www.digitaltown.co.uk/project6ESP32DCCAccDecodert.php
So main credits to digitaltown!

For my layout, this diagram with the 2x 2k2 resisters works perfectly.
Still need to experiment with other diagrams that might be better.

![DCC diagram using 6N137 to convert DCC signal to arduino](https://raw.githubusercontent.com/dkoster/esp32-dcc-acc-decoder/refs/heads/main/dcc-opto.png)
