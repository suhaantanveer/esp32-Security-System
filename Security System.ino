#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

const int green = 2;
const int red = 4;
const int buzzer = 19;
const int ir = 18;
const int button = 5;
const int screen_width = 128;
const int screen_length = 64;
const float FIRE_TEMP = 40.0;
const int max_length= 10;
const int max_string= 5;


const int DHTPIN = 33;
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

Adafruit_SSD1306 display(
  screen_width,
  screen_length,
  &Wire,
  -1
);

typedef enum{
  ARMED,
  ALARMED,
  DISARMED,
  FIRE_ALERT,
  NOTHING
}State;

typedef struct Node{
    State state;
    struct Node *next;
}Node;

typedef struct queue{
    Node *head;
    Node *tail;
    int size;
}Queue;

Queue* create_queue(){
    Queue *queue = new Queue;
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

bool isempty(Queue *queue){
    if (queue->size == 0){
        return true;
    }
    return false;
}

bool isfull(Queue *queue){
    if (queue->size == 5){
        return true;
    }
    return false;
}

void enqueue(Queue *queue, State word){
    Node *newNode = new Node;
    newNode->state = word;
    newNode->next = NULL;    
    if(isempty(queue)){
        queue->tail = newNode;
        queue->head = newNode;
        queue->size++;
    }
    else if(queue->size == 1){
        queue->tail->next = newNode;
        queue->head = newNode;
        queue->size++;
    }
    else if(!isfull(queue)){
        queue->head->next = newNode;
        queue->head = newNode;
        queue->size++;
    }
    else{
        queue->head->next = newNode;
        queue->head=newNode;
        Node *xy = queue->tail;
        queue->tail = xy->next;
        delete xy;
    }
}



State currentstate = ARMED;
State previousstate = currentstate;
Queue *queue;
int watch = 3;

void setup(){
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(ir, INPUT);
  pinMode(button, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("Alarm System has Started: ");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED Failed");
    while(true);
  }

  display.setTextColor(SSD1306_WHITE);
  dht.begin();
  queue = create_queue();
  enqueue(queue, currentstate);
  updateDisplay(3);
}


State dequeue(Queue *queue){
    if(!isempty(queue)){
        State out = queue->tail->state;
        Node *xy = queue->tail; 
        queue->tail= queue->tail->next;
        queue->size--;
        delete xy;
        return out;
    }
    else {
        State chonk = NOTHING;
        return chonk;
    }
}

void print_node(Node *node){
  Serial.println(stateToString(node->state));
  display.println(stateToString(node->state));
  if (node->next != NULL){
    print_node(node->next);
  }
}

void print_queue(Queue *queue){
  if(!isempty(queue)){
    print_node(queue->tail);
  }
  else {
    Serial.println("Nothing is in the queue");
    display.print("No logs");
  }
}

void print_node_serial(Node *node){
  Serial.println(stateToString(node->state));
  if (node->next != NULL){
    print_node(node->next);
  }
}

void print_queue_serial(Queue *queue){
  if(!isempty(queue)){
    print_node_serial(queue->tail);
  }
  else {
    Serial.println("Nothing is in the queue");
  }
}

void waitup(long time){
  long current = millis();
  while(millis() - current <= time){
    int buttonstate = digitalRead(button);
    if(buttonstate == LOW){
      currentstate = DISARMED;
      return;
    }
  }
}


State stringToState(String cmd) {
    if (cmd == "ARM") return ARMED;
    if (cmd == "ALARM") return ALARMED;
    if (cmd == "DISARM") return DISARMED;
    if (cmd == "FIRE ALERT") return FIRE_ALERT;

    return ARMED; 
}

void alarmcode(){
  digitalWrite(green, LOW);
  digitalWrite(red, HIGH);
  digitalWrite(buzzer , HIGH);
  waitup(1000);
  digitalWrite(red, LOW);
  waitup(1000);
}

void disalarmcode(){
  digitalWrite(buzzer, LOW);
  digitalWrite(green, HIGH);
  digitalWrite(red, LOW);
}

void checkState() {
  int buttonstate = digitalRead(button);
  int irstate = digitalRead(ir);
  float temp = dht.readTemperature();

  if((currentstate == ALARMED || currentstate == FIRE_ALERT) && buttonstate == LOW){
    currentstate = DISARMED;
  }

  if(currentstate == DISARMED && buttonstate == LOW){
    currentstate = ARMED;
  }

  if(currentstate == ARMED && irstate == LOW){
    currentstate = ALARMED;
  }
  if (currentstate == ARMED && temp >= FIRE_TEMP){
    currentstate = FIRE_ALERT;
  }
}


String stateToString(State state){
    if (state == ARMED) return "ARMED";
    if (state == ALARMED) return "ALARMED";
    if (state == DISARMED) return "DISARMED";
    if (state == FIRE_ALERT) return "FIRE_ALERT";

    return "ARMED";
}


void updateDisplay(int i){
  
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if(i==3){
    display.setTextSize(2);
    display.setCursor(0, 5);
    display.println("Menu:");
    display.setTextSize(1);
    display.setCursor(0, 25);
    display.println("1: Temp & Hum");
    display.setCursor(0, 40);
    display.println("2: State Logs");
    display.setCursor(0, 55);
    display.println("3: Back to Menu");
  }
  else if (i==2){
    display.setCursor(0, 5);
    display.setTextSize(2);
    display.println("STATE: ");
    display.setTextSize(1);
    print_queue(queue);
  }
  else if (i==1){
    display.setCursor(0, 5);
    display.setTextSize(1);
    display.print("Temp: ");
    display.print(temp);
    display.println("C");
    display.setCursor(0,20);
    display.print("Humidity: ");
    display.print(hum); 
    display.println("%");
  }
  else{
    display.println("Wrong input");
  }
}

void updatestate(String cmd){
  String cmd1 = "";
  String cmd2 = "";

  if (cmd.indexOf(' ') != -1) {
    int spacePosition = cmd.indexOf(" ");
    cmd1 = cmd.substring(0,spacePosition);
    cmd2 = cmd.substring(spacePosition+1);
  }
  if(cmd == "ALARM"){
    currentstate = stringToState(cmd);
  }

  else if (cmd == "print"){
    print_queue_serial(queue);
  }
  else if(cmd == "1" || cmd == "2" || cmd == "3"){
    watch= cmd.toInt();
    updateDisplay(watch);
  }
  else if (cmd1 == "DISARM" || cmd1 == "ARM"){
    if (cmd2 == "1234"){
      currentstate = stringToState(cmd1);
    }    
  }
}




void loop(){
  
  checkState();  

  if(currentstate != previousstate){
    enqueue(queue,currentstate);
    previousstate = currentstate;
  }

  if (Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    updatestate(cmd);
  }

  if (currentstate != previousstate){
      enqueue(queue,currentstate);
      previousstate=currentstate;
  }

  switch (currentstate){
    case ARMED:
      disalarmcode();
      break;
    case ALARMED:
      alarmcode();
      break;      
    case DISARMED:
      disalarmcode();
      break;
    case FIRE_ALERT:
      alarmcode();
      break;
  }
  display.clearDisplay();
  updateDisplay(watch);
  display.display();
  waitup(100);


}
