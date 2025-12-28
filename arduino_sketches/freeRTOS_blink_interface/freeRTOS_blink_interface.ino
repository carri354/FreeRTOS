//Restrict to 1 core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include <stdlib.h>

static const uint8_t buf_len = 20;
static const int ledPin = LED_BUILTIN;
static int blinkRate = 500; //in ms

void blink_LED(void *parameter){
  while(1){
    digitalWrite(ledPin,HIGH);
    vTaskDelay(blinkRate / portTICK_PERIOD_MS);
    digitalWrite(ledPin,LOW);
    vTaskDelay(blinkRate / portTICK_PERIOD_MS);
  }
}

void readSerial(void *parameter){
  char c;
  char buf[buf_len];
  uint8_t idx = 0;

  memset(buf,0,buf_len);
  while(1){
    if(Serial.available()>0){
      c = Serial.read();

      if(c == '\n'){
        blinkRate = atoi(buf);
        Serial.print("Updated LED delay to: ");
        Serial.println(blinkRate);
        memset(buf,0,buf_len);
        idx = 0;
      }else{
        if(idx < buf_len - 1){
          buf[idx] = c;
          idx++;
        }else{
          Serial.println("Buffer Overflow!");
        }
      }
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin,OUTPUT);
  Serial.begin(300);
  Serial.println("Please enter the blink rate into the serial monitor");

  xTaskCreatePinnedToCore(blink_LED,"Blink LED",1024,NULL,2,NULL,app_cpu);
  xTaskCreatePinnedToCore(readSerial,"Set Rate",1024,NULL,1,NULL,app_cpu);
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
