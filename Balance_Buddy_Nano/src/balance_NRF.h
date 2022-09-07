class BalanceNRF {
    private:

        struct SEND_DATA_STRUCTURE {
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

        SEND_DATA_STRUCTURE send_data;

        int ADC_pin_1 = 3;
        int ADC_pin_2 = 9;

        int switch_val_index = 0;
        int num_vals = 2;
        int switch_vals[10];
        int switch_sum=0;

        long currenttime = 0;
        long lastreceive = 0;
        long lastreceive_interval = 1000;

        int pad1_pin = A0;
        int pad2_pin = A1;
        int pad1_val = 0;
        int pad2_val = 0;

    public:

        int deadman_switch = 0;
        int last_deadman = 0;
        bool deadman_switch_change = false;
        bool arm_alarm = false;

        bool tx_ok;
        bool tx_failed;
        bool rx_ready;

        struct RECEIVE_DATA_STRUCTURE {
            int switch_state; // 0 = fault, 1 = active
            bool request_data;
            int led_brightness;
            int checksum;
            int mode; // 0 - remote,  1 - footpad
        };

        RECEIVE_DATA_STRUCTURE receive_data;
        RECEIVE_DATA_STRUCTURE rd_check;

        void setup() {
            pinMode(ADC_pin_1, OUTPUT);
            pinMode(ADC_pin_2, OUTPUT);
            pinMode(pad1_pin, INPUT);
            pinMode(pad2_pin, INPUT);
            Radio_Setup(4);
            receive_data.led_brightness = 1023;
            receive_data.mode = 1;
        }

        void Radio_Setup(int PA_level) {
            radio.begin();
            radio.setPayloadSize(sizeof(send_data));
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
            radio.openWritingPipe(address[1]);
            radio.openReadingPipe(1, address[0]);
        }

        // send data = 0, receive data = 1
        int Calc_Checksum(int type) {
            unsigned int calc_checksum = 0;
            unsigned int checksum = 0;
            // pack for sending
            if (type == 0) {
                send_data.checksum = 0;
                unsigned char *p = (unsigned char *)&send_data;
                for (unsigned int i=0; i<sizeof(send_data); i++) {
                    calc_checksum += p[i];
                }
            // check receiving
            } else if (type == 1) {
                checksum = rd_check.checksum;
                rd_check.checksum = 0;
                unsigned char *p = (unsigned char *)&rd_check;
                for (unsigned int i=0; i<sizeof(rd_check); i++) {
                    calc_checksum += p[i];
                }
                rd_check.checksum = checksum;
            }
            return calc_checksum;
        }
        
        void Receive_Data() {
            radio.startListening();
            // radio.openReadingPipe(0, address);
            if (radio.available()) {
                radio.read(&rd_check, sizeof(rd_check));
                if (rd_check.checksum == Calc_Checksum(1)) {
                    arm_alarm = false;
                    receive_data.switch_state = rd_check.switch_state;
                    receive_data.led_brightness = rd_check.led_brightness;
                    receive_data.request_data = rd_check.request_data;
                    receive_data.mode = rd_check.mode;
                    lastreceive = currenttime;

                } else {
                    Serial.println("Received Bad Data");
                    arm_alarm = true; 
                }
                // data output in "receive_data.XXXX"
            }
        }   

        void Send_Data() {
            delay(1);
            send_data.tempMosfet = esc.tempMosfet*100.0;
            send_data.dutyCycle = esc.dutyCycle*1000.0;
            send_data.voltage = esc.voltage*100.0;
            send_data.balanceState = esc.balanceState;
            send_data.switchState = esc.switchState;
            send_data.adc1 = esc.adc1*100.0;
            send_data.adc2 =esc.adc2*100.0;
            send_data.erpm = esc.erpm;
            send_data.motorCurrent = esc.motorCurrent*100.0;
            send_data.pitch = esc.pitch*100.0;
            send_data.roll = esc.roll*100.0;
            send_data.checksum = Calc_Checksum(0);

            // Serial.println(send_data.pitch);

            radio.stopListening();
            // radio.openWritingPipe(address);
            radio.write(&send_data, sizeof(SEND_DATA_STRUCTURE));
            
            delay(1);
        }

        void Deadman_Filter(){
            switch_vals[switch_val_index] = receive_data.switch_state;
            if (switch_val_index < (num_vals-1)) {
                switch_val_index++;
            } else {
                switch_val_index = 0;
            }
            
            for(int i=0; i<num_vals; i++) {
                switch_sum += switch_vals[i];
            }

            if (switch_sum == num_vals) {
                deadman_switch = 1;
            } else if (switch_sum == 0) {
                deadman_switch = 0;
            }
            switch_sum = 0;

            if(deadman_switch != last_deadman) {
                deadman_switch_change = true;
            } else {
                deadman_switch_change = false;
            }
            last_deadman = deadman_switch;
        }

        void loop() {
            currenttime = millis();
            // Deadman_Filter(); // moved to below
            Receive_Data();
            // Serial.println(receive_data.switch_state);


            if (receive_data.mode == 0) {                
                // send activation signal to cFOCER - FROM REMOTE
                // filter signal from remote
                Deadman_Filter();

                if (deadman_switch == 1) {
                    analogWrite(ADC_pin_1, 175);
                    analogWrite(ADC_pin_2, 175);
                } else {
                    digitalWrite(ADC_pin_1, 0);
                    digitalWrite(ADC_pin_2, 0);
                }
            } else {
                // use footpads for activation
                pad1_val = analogRead(pad1_pin);
                pad2_val = analogRead(pad2_pin);

                analogWrite(ADC_pin_1, map(pad1_val, 0, 1023, 0, 175));
                analogWrite(ADC_pin_2, map(pad2_val, 0, 1023, 0, 175));

                // for debugging
                // Serial.print("Pad1: ");
                // Serial.print(pad1_val);
                // Serial.print("\tPad2: ");
                // Serial.print(pad2_val);
                // Serial.println();
            }


            if (receive_data.request_data && (currenttime - lastreceive) <= lastreceive_interval) {
                Send_Data();
                arm_alarm = false;
            } else if ((currenttime - lastreceive) > lastreceive_interval) {
                receive_data.mode = 1; // added 8-19-2022
                // arm_alarm = true; // should it beep when this happens?
                radio.whatHappened(tx_ok, tx_failed, rx_ready);
                Serial.print("TX OK "); Serial.print(tx_ok); Serial.print(" TX Failed: "); Serial.print(tx_failed); Serial.print(" RX Ready: "); Serial.println(rx_ready);
                radio.flush_rx();
                radio.flush_tx();                
                radio.openWritingPipe(address[1]);
                radio.openReadingPipe(1, address[0]);            
            }
        }
};








