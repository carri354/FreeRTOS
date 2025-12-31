//Restrict to 1 core

static const BaseType_t pro_cpu = 0;

static const BaseType_t app_cpu = 1;


// Include
#include <stdlib.h>


// Internal clock runs at 80 MHz
// Settings

static const uint32_t cli_delay = 20; // in ms
static const uint32_t timer_freq = 10000000; // Timer runs at 10MHz
static const uint64_t timer_max_count = 1000000; // Triggers at 1s
static const char command[] = "avg";
enum {BUF_LEN = 20};
enum {MSG_QUEUE_LEN = 5};
enum {MSG_SIZE = 100};
enum {CMD_BUF_LEN = 255};

// Message struct to wrap strings for queue
typedef struct Message {
  char body[MSG_SIZE];
} Message;

// Pins
static const int analog_pin = A0;

//Globals
static hw_timer_t *timer = NULL;
// Need volatile keyword to indicate changes in an ISR
static volatile uint16_t buf0[BUF_LEN];
static volatile uint16_t buf1[BUF_LEN];
static volatile uint16_t *readFrom = buf1;
static volatile uint16_t *writeTo = buf0;
static volatile uint8_t buffer_overrun = 0;

static float adc_avg;

static SemaphoreHandle_t doneReading_sem = NULL;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t msg_queue = NULL;

static TaskHandle_t h_calcAvg = NULL;
// Interrupt Service Routines

void IRAM_ATTR swapBuffers(){
  volatile uint16_t *temp = readFrom;
  readFrom = writeTo;
  writeTo = temp;
}

void IRAM_ATTR sampleADC(){
  BaseType_t task_woken = pdFALSE;
  static uint16_t idx = 0;
  // Add a sample
  if((idx < BUF_LEN) && !(buffer_overrun)){
    writeTo[idx] = analogRead(analog_pin);
    idx++;
  }
  // Check if buffer is full
  if(idx >= BUF_LEN){
    // If calcAvg is not done reading, buffer is overflowing
    if(xSemaphoreTakeFromISR(doneReading_sem,&task_woken) == pdFALSE){
        buffer_overrun = 1;
    }
    if(buffer_overrun == 0){
      idx = 0;
      swapBuffers();
      vTaskNotifyGiveFromISR(h_calcAvg,&task_woken);
    }
  }
  if (task_woken) {
    portYIELD_FROM_ISR();
  }
}


// Tasks

/*
* Print any new messages
* Save characters to buffer
* Watch for avg command
*/
void doCLI(void *parameter){
  char c;
  uint32_t idx = 0;
  char buf[CMD_BUF_LEN];
  Message rcv_msg;
  //int temp; Uncomment to test analog reads

  //Clear buffer
  memset(buf,0,CMD_BUF_LEN);

  while(1){
    /*Uncomment to test ananlog reads
    temp = analogRead(analog_pin);
    Serial.println(temp);
    vTaskDelay(500/portTICK_PERIOD_MS);
    */
    if(xQueueReceive(msg_queue,(void *)&rcv_msg,0) == pdTRUE){
      Serial.println(rcv_msg.body);
    }

    if(Serial.available() > 0 ){
      c = Serial.read();
      //Only update buffer if it has space
      if(idx < CMD_BUF_LEN-1){
        buf[idx] = c;
        idx++;
      }

      if(c == '\n'){
        Serial.println();
        buf[idx-1] = '\0';
        
        // Check if message is "avg"
        if(strcmp(buf,command) == 0){
          Serial.print("Average: ");
          Serial.println(adc_avg);
        }
        // Clear buffer
        memset(buf,0,CMD_BUF_LEN);
        idx = 0;
      }else{
        //Echo characters back
        Serial.print(c);
      }
    }
    //Yield to other task (NOT NEEDED IF CLI IS LOWER PRIORITY)
    //vTaskDelay(cli_delay / portTICK_PERIOD_MS);
  }
}


// Should be dormant until signal from ISR
void calcAverage(void *parameter){
  Message msg;
  float avg;
  //Set up interrupt
  timer = timerBegin(timer_freq);
  timerAttachInterrupt(timer, sampleADC);
  timerAlarm(timer, timer_max_count,true,0);
  timerStart(timer);

  vTaskDelete(NULL);
  while(1){
    //strcpy(msg.body,"I'm running!");
    //xQueueSend(msg_queue,&msg,10);

    // Wait for notification from ISR
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    
    //strcpy(msg.body,"ISR was triggered!");
    //xQueueSend(msg_queue,&msg,10);
    
    // Begin calculating average
    avg = 0.0;
    // Not a critical section because ISR will be working on the other buffer
    for(int i = 0;i<BUF_LEN;i++){
      avg += (float)readFrom[i];
    }
    
    avg /= BUF_LEN;

    //Critical section: copying avg to adc_avg
    portENTER_CRITICAL(&spinlock);
    adc_avg = avg;
    portEXIT_CRITICAL(&spinlock);

    // Notify CLI about overflow
    if(buffer_overrun == 1){
      strcpy(msg.body,"Buffer Overflow. Samples Dropped!");
      xQueueSend(msg_queue,&msg,10);
    }

    //Critical section: doneReading and buffer_overrun are closely linked in ISR
    portENTER_CRITICAL(&spinlock);
    xSemaphoreGive(doneReading_sem);
    buffer_overrun = 0;
    portEXIT_CRITICAL(&spinlock);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(analog_pin,INPUT);
  // Create Semaphores and Queue
  doneReading_sem = xSemaphoreCreateBinary();
  msg_queue = xQueueCreate(MSG_QUEUE_LEN,sizeof(Message));

  vTaskDelay(1000/portTICK_PERIOD_MS);
  
  if(doneReading_sem == NULL){
    Serial.println("Could not create semaphores, restarting...");
    ESP.restart();
  }

  Serial.println("---Hardware Interrupt Showcase---");
  // Start done reading at 1 to allow for buffer swap
  xSemaphoreGive(doneReading_sem);
  
  // Create Tasks 
  xTaskCreatePinnedToCore(doCLI,"Command-Line Interface",2048,NULL,1,NULL,app_cpu);
  xTaskCreatePinnedToCore(calcAverage,"Calculate Average ADC value",1024,NULL,2,&h_calcAvg,pro_cpu);

  
}

void loop() {
  // put your main code here, to run repeatedly:
  

}
