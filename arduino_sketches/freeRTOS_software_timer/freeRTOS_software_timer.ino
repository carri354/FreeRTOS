//Restrict to 1 core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static TickType_t led_timeout = 1000/portTICK_PERIOD_MS; // How many ticks the timer will expire in (convert ms to ticks!)


// Globals
const static int led_pin = LED_BUILTIN;
TimerHandle_t hled_timer;

void timeout( TimerHandle_t xTimer ){
  digitalWrite(led_pin,LOW);
}

void echo(void *parameter){
  char c;
  while (1){

    // See if there are things in the input serial buffer
    if (Serial.available() > 0) {

      // If so, echo everything back to the serial port
      c = Serial.read();
      Serial.print(c);

      // Turn on the LED
      digitalWrite(led_pin, HIGH);

      // Start timer (if timer is already running, this will act as
      // xTimerReset() instead)
      xTimerStart(hled_timer, portMAX_DELAY);
    }
  }
}



void setup(){
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(led_pin,OUTPUT);

  vTaskDelay(1000/ portTICK_PERIOD_MS);

  Serial.println("---Software Timer Showcase---");
  // Initialize timer handle
  hled_timer = xTimerCreate("LED Timer", led_timeout,pdFALSE,(void *)0,timeout);

  if(hled_timer == NULL){
    Serial.println("Timer could not be created");
  }else{
    xTaskCreatePinnedToCore(echo,"Keep on",1024,NULL,1,NULL,app_cpu);
  }

  vTaskDelete(NULL);
}

void loop(){
  // put your main code here, to run repeatedly:

}
