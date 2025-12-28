//Restrict to 1 core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif



const int led_pin = LED_BUILTIN;
void toggleLED(void *parameter){
  while(1){
    digitalWrite(led_pin,HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(led_pin,LOW);
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

void secondBlink(void *parameter){
  while(1){
    digitalWrite(led_pin,HIGH);
    vTaskDelay(1200 / portTICK_PERIOD_MS);
    digitalWrite(led_pin,LOW);
    vTaskDelay(1200/portTICK_PERIOD_MS);
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(led_pin,OUTPUT);
  digitalWrite(led_pin,LOW);

  //Specify task to run in only one core
  //Regular FreeRTOS just uses xTaskCreate()
  //Params: function to be called, task name, stack size (in bytes/ words in regular RTOS),
  //parameter to selected function, task priority, task handle, which core to use
  xTaskCreatePinnedToCore(toggleLED,"Toggle LED (1)",1024,NULL,1,NULL,app_cpu);
  xTaskCreatePinnedToCore(secondBlink,"Toggle LED (2)",1024,NULL,2,NULL,app_cpu);
  
  //In regular FreeRTOS, need to call vTaskStartScheduler after setting up tasks
}

void loop() {
  // put your main code here, to run repeatedly:

}
