#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>

#define DEBUG

Preferences preferences;

enum status_e{UNKNOWN, RED, GREEN}; 

#ifdef DEBUG
    RTC_DATA_ATTR int bootCount = 0;
#endif
RTC_DATA_ATTR uint16_t threshold = 0;
RTC_DATA_ATTR enum status_e status;

RTC_DATA_ATTR uint16_t measBuffer[24];
RTC_DATA_ATTR uint16_t thresholdBuffer[24];
RTC_DATA_ATTR uint32_t timestampBuffer[24];
RTC_DATA_ATTR uint8_t bufferIndex = 0;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("WIFI_SSID", "WIFI_PASSWORD");
  #ifdef DEBUG
    Serial.print("Connecting to WiFi ..");
  #endif
  uint8_t timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    timeout++;
    if (timeout==10)
        ESP.restart();
  }
  #ifdef DEBUG
    Serial.println(WiFi.localIP());
  #endif
}

void setup() {
    
  // put your setup code here, to run once:
  pinMode(GPIO_NUM_13,OUTPUT);
  pinMode(GPIO_NUM_27,OUTPUT);
  pinMode(GPIO_NUM_26,INPUT_PULLUP);
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_config(TOUCH_PAD_NUM2, 0);
  
  //do measurement 
  uint16_t output;
  touch_pad_read(TOUCH_PAD_NUM2,&output);

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : 
        #ifdef DEBUG
            Serial.println("Wakeup caused by external signal using RTC_IO"); 
        #endif

        //debounce button press
        delay(1000);

        //set threshold to current value      
        threshold = output - 5;
        preferences.begin("nvm", false); 
        preferences.putUShort("threshold",threshold);
        preferences.end();

        if (status!=RED)
        {
            //set to red
            digitalWrite(GPIO_NUM_13,HIGH);
            delay(500);
            digitalWrite(GPIO_NUM_13,LOW); 
            status = RED;
        }

        break;
    case ESP_SLEEP_WAKEUP_TIMER : 
        #ifdef DEBUG
            Serial.println("Wakeup caused by timer"); 
        #endif

        //write buffers
        time_t now;
        time(&now);
        timestampBuffer[bufferIndex] = now;
        thresholdBuffer[bufferIndex] = threshold;
        measBuffer[bufferIndex] = output;
        bufferIndex++;

        if (bufferIndex == 24 || (output > threshold && status!=RED))
        {
            //write to DB daily or when watering is needed            
            initWiFi();

            HTTPClient http;
            http.begin("http://192.168.1.90:8086/write?db=plant&u=admin&p=admin");
            http.addHeader("Content-Type", "application/binary");

            char payload[2000], *pos = payload;

            for(int i=0; i<bufferIndex; i++)
            {
                pos += sprintf(pos,"moisture2 level=%d,threshold=%d %d000000000\n",measBuffer[i],thresholdBuffer[i],timestampBuffer[i]);
            }    

            http.POST((uint8_t *)payload, strlen(payload));
            http.end();

            bufferIndex=0;     
        }

        //check the sensor
        if (output > threshold && status!=RED)
        {
            //set to red
            digitalWrite(GPIO_NUM_13,HIGH);
            delay(500);
            digitalWrite(GPIO_NUM_13,LOW); 
            status = RED;
        } 
        else if (output <= threshold && status!=GREEN)  
        {
            //set to green
            digitalWrite(GPIO_NUM_27,HIGH);
            delay(500);
            digitalWrite(GPIO_NUM_27,LOW);
            status = GREEN;
        }

        break;
    default : 
        //do not check the sensor, since the battery is just placed, it can be that it is not placed in the soil
        #ifdef DEBUG
            Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); 
        #endif

        //read EEPROM
        preferences.begin("nvm", true); 
        threshold = preferences.getUShort("threshold");
        preferences.end();

        //set real time clock
        initWiFi();
        configTime(0, 0, "pool.ntp.org");   
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        
        break;
  }

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0);
  #ifdef DEBUG
    // esp_sleep_enable_timer_wakeup(5000000);
    esp_sleep_enable_timer_wakeup(3600000000);
  #else
    esp_sleep_enable_timer_wakeup(3600000000);
  #endif

  #ifdef DEBUG
    Serial.printf("Sensor value: %d\n",output);
    Serial.printf("Threshold value: %d\n",threshold);
    Serial.printf("Cycle count: %d\n",bootCount);  
    bootCount++;
  #endif

  esp_deep_sleep_start();

}

void loop() {

  // no loop

}