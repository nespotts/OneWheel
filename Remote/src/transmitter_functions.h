struct SEND_DATA_STRUCTURE{
  int switch_state; // 0 = fault, 1 = active
  bool request_data;
  int led_brightness;
  int checksum;
  int mode; // 0 - remote, 1 - footpad
};

SEND_DATA_STRUCTURE send_data;

// Struct for transmitting data (all INTs)
struct RECEIVE_DATA_STRUCTURE_INT{
  int tempMosfet;
  int dutyCycle;
  int voltage;
  int balanceState;
  int switchState;
  int adc1;
  int adc2;
  long erpm;
  int motorCurrent;
  int pitch;
  int roll;  
  int checksum;
};

RECEIVE_DATA_STRUCTURE_INT rd;

// Struct for containing floats of data received
struct RECEIVE_DATA_STRUCTURE_FLOAT{
  float tempMosfet;
  float dutyCycle;
  float voltage;
  int balanceState;
  int switchState;
  float adc1;
  float adc2;
  long erpm;
  float motorCurrent;
  float pitch;
  float roll;  
};

RECEIVE_DATA_STRUCTURE_FLOAT rdf;

void Deadman_Filter(){
  switch_vals[switch_val_index] = digitalRead(switch_pin);
  switch_val = switch_vals[switch_val_index];

  // Increment Index
  if (switch_val_index < (num_vals-1)) {
    switch_val_index++;
  } else {
    switch_val_index = 0;
  }
  
  // Calculate sum
  for(int i=0; i<num_vals; i++) {
    switch_sum += switch_vals[i];
  }

  // Set deadman Switch Value
  if (switch_sum == num_vals) {
    deadman_switch = 1;
  } else if (switch_sum == 0) {
    deadman_switch = 0;
  }
  switch_sum = 0;

  // set flag if deadman_switch changed value
  // if (deadman_switch != last_deadman) {
  if (rdf.switchState != last_deadman) {
      deadman_switch_change = true;
  } else {
      deadman_switch_change = false;
  }
  last_deadman = rdf.switchState;

  // Set flag first time deadman switch value changes - used to stop requests and display updates
  if (switch_val != last_switch_val) {
    request_timer += 250;
    display_timer += 250;
  }
  last_switch_val = switch_val;
}

// send data = 0, receive data = 1
int Calc_Checksum(int type) {
  unsigned int checksum = 0;
  if (type == 0) {
    send_data.checksum = 0;
    unsigned char *p = (unsigned char *)&send_data;
    for (unsigned int i=0; i<sizeof(send_data); i++) {
        checksum += p[i];
    }
  } else if (type == 1) {
    rd.checksum = 0;
    unsigned char *p = (unsigned char *)&rd;
    for (unsigned int i=0; i<sizeof(rd); i++) {
        checksum += p[i];
    }
  }
  return checksum;
}

void SendData() {
  delay(1);
  // radio.openWritingPipe(address);
  radio.stopListening();
  send_data.switch_state = deadman_switch;
  // Serial.println(send_data.switch_state);
  send_data.checksum = Calc_Checksum(0);
  // Serial.println(send_data.checksum);
  radio.write(&send_data, sizeof(SEND_DATA_STRUCTURE));
  delay(1);
}

void ReceiveData() {
  if (send_data.request_data) {
    radio.startListening();
    // radio.openReadingPipe(0, address);

    // only wait 0.01 second to get response    
    long t1 = millis();
    while(!radio.available()) {
      long t2 = millis();
      if (t2 - t1 >= 3) {  // 30
        break;
      }
      // Serial.println("Waiting for Response");
    }
      
    if (radio.available()) {
      radio.read(&rd, sizeof(rd));

      // Check checksum - not necessary as this is not crucial data.
      if (rd.checksum == Calc_Checksum(1)) {
        // set timer for last time data received from OneWheel
        request_timer = currenttime;

        // Convert values to floats
        rdf.tempMosfet = (float)rd.tempMosfet / 100.0;
        rdf.dutyCycle = (float)rd.dutyCycle / 1000.0;
        rdf.voltage = (float)rd.voltage / 100.0;
        rdf.balanceState = rd.balanceState;
        rdf.switchState = rd.switchState;
        rdf.adc1 = (float)rd.adc1 / 100.0;
        rdf.adc2 = (float)rd.adc2 / 100.0;
        rdf.erpm = rd.erpm;
        rdf.motorCurrent = (float)rd.motorCurrent / 100.0;
        rdf.pitch = (float)rd.pitch / 100.0;
        rdf.roll = (float)rd.roll / 100.0;
      } else {
        Serial.println("Received Bad Data");
      }
    } else {
      // Serial.println("Radio Not Available");
    }
  }
}

void Radio_Setup(int PA_level) {
  radio.begin();
  radio.setPayloadSize(sizeof(rd));
  radio.setCRCLength(RF24_CRC_16);
  Serial.print("Payload Size: "); Serial.println(radio.getPayloadSize());
  Serial.print("CRC Length: "); Serial.println(radio.getCRCLength());

  if (PA_level == 1) {
      radio.setPALevel(RF24_PA_MIN);
  } else if (PA_level == 2) {
      radio.setPALevel(RF24_PA_LOW);
  } else if (PA_level == 3) {
      radio.setPALevel(RF24_PA_HIGH);
  } else {
      radio.setPALevel(RF24_PA_MAX);
  }
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);
  send_data.mode = 1;
}