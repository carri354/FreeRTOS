//Restrict to 1 core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const char msg[] = "Hello to everyone in the world!";

static TaskHandle_t task1 = NULL;
static TaskHandle_t task2 = NULL;

void message1(void *parameter){
  while(1){
    Serial.println();
    int msg_len = strlen(msg);
    for(int i =0;i<msg_len;i++){
      Serial.print(msg[i]);
    }
    Serial.println();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  
}

void message2(void *parameter){
  while(1){
    Serial.print("$");
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
  
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(300);

  vTaskDelay(1000/ portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("Priority example");

  xTaskCreatePinnedToCore(message1,"Low Priority Message", 1024,NULL,1,&task1,app_cpu);
  xTaskCreatePinnedToCore(message2,"High Priority Message", 1024, NULL,2,&task2,app_cpu);

}

void loop() {
  // put your main code here, to run repeatedly:
  
  for(int i=0;i<3;i++){
    vTaskSuspend(task2);
    vTaskDelay(2000/portTICK_PERIOD_MS);
    vTaskResume(task2);
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }

  if(task1 != NULL){
    vTaskDelete(task1);
    task1 = NULL;
  }
}
