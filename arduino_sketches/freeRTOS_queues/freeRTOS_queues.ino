#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

#include <stdlib.h>

//Settings 
static const uint8_t delay_queue_len = 5;
static const uint8_t msg_queue_len = 5;
static const char command[] = "delay ";
static const uint8_t blink_threshold = 50; //Blinks before a message is sent out
static const int buf_len = 255;

//Globals
static QueueHandle_t delay_queue; //Queue 1
static QueueHandle_t msg_queue; //Queue 2
static const uint8_t led_pin = LED_BUILTIN;

void CLI(void *parameter){
  char rcv_msg[20];
  char c;
  char buf[buf_len];
  int idx = 0;
  uint8_t cmd_len = strlen(command);

  memset(buf,0,buf_len);
  while(1){
    //First print any messages from Queue 2
    if(xQueueReceive(msg_queue,(void *)rcv_msg,0) == pdTRUE){
      Serial.println(rcv_msg);
    }

    //Read user input
    if(Serial.available()>0){
      c = Serial.read();
      if (idx < buf_len - 1) {
        buf[idx] = c;
        idx++ ;
      }

      if(c == '\n'){
      //Check for delay command
        if(memcmp(buf,command,cmd_len) == 0){
          buf[idx] = '\0'; //Turn buf into an actual string
          char *tail = buf + cmd_len;
          //Convert delay string to an integer
          int item = atoi(tail);
          //Store delay value in Queue 1
          if(xQueueSend(delay_queue,(void *)&item,10) == pdFALSE){
            Serial.println("Too many requests!");
          }
        }
        //Clear buffer and idx
        memset(buf,0,buf_len);  
        idx = 0;
        Serial.println();
      }else{
        //Echo input
        Serial.print(c);
      }
    }
  }
}

void blink(void *parameter){
  int t = 500; //milliseconds
  int blink_count = 0;
  char alert[14] = "Hello Friend!";

  while(1){
    xQueueReceive(delay_queue,(void *)&t,0);

    //Blink LED and count
    digitalWrite(led_pin,HIGH);
    vTaskDelay(t / portTICK_PERIOD_MS);
    digitalWrite(led_pin,LOW);
    vTaskDelay(t / portTICK_PERIOD_MS);
    blink_count++;

    if(blink_count >= blink_threshold){
      //Send message and reset count
      xQueueSend(msg_queue,(void *)alert,10);
      blink_count = 0;
    }

  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(led_pin,OUTPUT);
  vTaskDelay(1000/portTICK_PERIOD_MS);

  Serial.println("\nWelcome to the CLI!");
  Serial.println("Please type 'delay xxx' to change the LED");

  //Create Queues
  delay_queue = xQueueCreate(delay_queue_len,sizeof(int));
  msg_queue = xQueueCreate(msg_queue_len,sizeof(char)*50);
  //Create Tasks
  xTaskCreatePinnedToCore(CLI,"Command-line interface",2048,NULL,1,NULL,app_cpu);
  xTaskCreatePinnedToCore(blink,"Blink LED",2048,NULL,1,NULL,app_cpu);
  vTaskDelete(NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
