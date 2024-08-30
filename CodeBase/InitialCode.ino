//include libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

//define the parameters of OLED 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define NTP_SERVER     "pool.ntp.org" //server adress
#define UTC_OFFSET     0  //time zone
#define UTC_OFFSET_DST 0  //day light saving settings

#define BUZZER  5
#define LED_1 15
#define LED_2 14
#define LED_3 4
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

//Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,OLED_RESET);
DHTesp  dhtSensor;

//global variables
//initially all the time parameters are 0
int days = 0;
int hours=0;
int minutes=0;
int seconds=0;
int utc_offset = UTC_OFFSET;

unsigned long timeNow = 0; //as time is a big value to represent we use long format and also time is always positive, so we used unsigned for it.
unsigned long timeLast= 0; //this is essential to calculate current time

bool alarm_enabled=true;
int n_alarms=3; //to keep track on number of alarms
int alarm_hours[3];
int alarm_minutes[3];

bool alarm_triggered[] = {false,false,false}; //to check whether alarm has been triggered and user responded

int n_notes=8;
int C= 262;
int D= 294;
int E= 330;
int F= 349;
int G= 392;
int A=440;
int B= 494;
int C_H= 523;
int notes[]={C,D,E,F,G,A,B,C_H};

int current_mode = 0;
int max_modes = 5;
String modes[]= {"1- Set Time", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Set Alarm 3","5 - Disable alarms"};


void setup() {
  // put your setup code here, to run once:
  pinMode(BUZZER,OUTPUT);
  pinMode(LED_1,OUTPUT);
  pinMode(LED_2,OUTPUT);
  pinMode(LED_3,OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_DOWN, INPUT);
  Serial.begin(115200);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  // SSD1306_SWITCHCAPVCC - generate dispaly voltage from 3.3 V internally
  //implement if clause to check whether the display is initially working properly

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); //implement an infinite loop
  }

  //show the display buffer on the screen. You MUST call display() after
  //drawing commands to make them visible on screen!
  display.display();
  delay(2000);
  
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to Wi-Fi",0,0,2);
    
  }
  display.clearDisplay();
  print_line("Connected to Wi-Fi",0,0,2);
  

  configTime(utc_offset * 3600,0, NTP_SERVER);
  //clear the buffer
  display.clearDisplay();

  print_line("Welcome to Medibox", 10,20,2);
  delay(2000);
  display.clearDisplay();
   
  set_utc_offset();

}

void loop() {

  // put your main code here, to run repeatedly:
  update_time_with_set_alarms();
  if(digitalRead(PB_OK)==LOW){ //direct the user to menu if he pressed OK
    delay(200);//to debounce the push button
    go_to_menu();
  }
  check_temp();
  
}


//This function prints the text we want in the display in the required place mentioned(column,row)
void print_line(String text, int column , int row ,int text_size){
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column,row); //(column,row)
  display.println(text);

  display.display();
}

void print_time_now(void){
  //here 10 pixels for 1 character. So integer values of days take 2 characters at maximum. So, 20 pixels are enough.
  display.clearDisplay();//this prevents time is printing over itself
  print_line("Date:",0,0,2);
  print_line(String(days),60,0,2);
  print_line(String(hours),0,20,2);
  print_line(":",30,20,2);
  print_line(String(minutes),40,20,2);
  print_line(":",70,20,2);
  print_line(String(seconds),80,20,2);
  
}

void update_time(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3,"%H", &timeinfo);
  hours= atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3,"%M", &timeinfo);
  minutes= atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3,"%S", &timeinfo);
  seconds= atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3,"%d", &timeinfo);
  days= atoi(timeDay);
}


void ring_alarm(){
  display.clearDisplay();
  print_line("Medicine Time",0,0,2);

  digitalWrite(LED_1,HIGH);


  bool break_happened = false;//this is to handle situations like user does not push the button very fast.

  //ring the buzzer
  while( break_happened == false && digitalRead(PB_CANCEL)==HIGH){
    for(int i=0;i<n_notes;i++){
      if(digitalRead(PB_CANCEL) == LOW){
        delay(200); // prevent bouncing the push button
        break_happened = true; // this true if the button is pressed only do not want to release it
        break;
      }
      tone(BUZZER,notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }    
    
  digitalWrite(LED_1,LOW);
  display.clearDisplay();
}

void update_time_with_set_alarms(void){
  update_time();
  print_time_now();

  if(alarm_enabled==true){
    for(int i=0;i<n_alarms;i++){
      if(alarm_triggered[i]==false && alarm_hours[i]==hours && alarm_minutes[i]==minutes){
        ring_alarm();
        alarm_triggered[i]=true; // this is to stop the alarm after user pressed the button. without this if the user press the alarm at the same minute the alarm beeps, alarm will again start to beep.
      }
    }

  }

}

int wait_for_button_press(){
  while(true){ //infinite loop until  the button press
    if(digitalRead(PB_UP)==LOW){
      delay(200);
      return PB_UP; // this return the value of the input as well as going out of the infinite loop
    }
    else if(digitalRead(PB_DOWN)==LOW){
      delay(200);
      return PB_DOWN; 
    }
    else if(digitalRead(PB_OK)==LOW){
      delay(200);
      return PB_OK; 
    }
    else if(digitalRead(PB_CANCEL)==LOW){
      delay(200);
      return PB_CANCEL; 
    }
    
    
  }
}

void go_to_menu(){
  while(digitalRead(PB_CANCEL)== HIGH){
  //this while loop will execute until the user press cancel button
    display.clearDisplay();
    print_line(modes[current_mode],0,0,2);

    int pressed= wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      current_mode +=1;
      current_mode = current_mode% max_modes;
    }
    else if(pressed == PB_DOWN){
      delay(200);
      current_mode -=1;
      if(current_mode<0){
        current_mode=max_modes-1;
      }
    }
    else if(pressed == PB_OK){
      delay(200);
      run_mode(current_mode);

    }
    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }

  }
  
}

void set_time(){
  int temp_hour = hours;


  while(true){
    display.clearDisplay();
    print_line("Enter hour: "+ String(temp_hour),0,0,2);
  
    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      temp_hour +=1;
      temp_hour = temp_hour % 24;
    }
    else if(pressed == PB_DOWN){
      delay(200);
      temp_hour -=1;
      if(temp_hour<0){
        temp_hour=23;
      }
    }
    else if(pressed == PB_OK){
      delay(200);
      hours= temp_hour;
      break;

    }
    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }

  }

  int temp_minute = minutes;
  while(true){
    display.clearDisplay();
    print_line("Enter minute: "+ String(temp_minute),0,0,2);
  
    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      temp_minute +=1;
      temp_minute= temp_minute % 60;
    }
    else if(pressed == PB_DOWN){
      delay(200);
      temp_minute -=1;
      if(temp_minute<0){
        temp_minute=59;
      }
    }
    else if(pressed == PB_OK){
      delay(200);
      hours= temp_minute;
      break;

    }
    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }

  }
  display.clearDisplay();
  print_line("Time is set",0,0,2);
  delay(1000);
  
}
void set_alarm(int alarm) {
  alarm_enabled = true; //avoid disabling alarms permanently after pressing disable alarms
  int temp_hour = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      alarm_triggered[alarm] = false; // Reset the alarm trigger flag
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
 
  }
  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
}

void run_mode(int mode){
  if (mode==0){
    set_time();
  }

  else if (mode ==1 || mode == 2 || mode==3){
    alarm_enabled=true;
    set_alarm(mode-1);
  }
  else if(mode== 4){
    display.clearDisplay();
    print_line("Alarms disabled",0,0,2);
    alarm_enabled = false;
  }
  
  

}

void check_temp(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();//those functions can be found in documentation of library
  //this data type is not a common arduino data type. This is belong to DHT22 library 
  //implement red LED to signal that one or both of the parametrs are more than the range.
  //implement green LED to signal that one or both of the parametrs are less than the range
  if (data.temperature > 32){
    digitalWrite(LED_2,HIGH);
    digitalWrite(LED_3,LOW);
    print_line("Temp. High",0,40,1);
    delay(500);
    
  }
  else if (data.temperature < 26){
    digitalWrite(LED_3,HIGH);
    digitalWrite(LED_2,LOW);
    print_line("Temp. LOW",0,40,1);
    delay(500);
    
  }
  if (data.humidity > 80){
    digitalWrite(LED_2,HIGH);
    digitalWrite(LED_3,LOW);
    print_line("Humidity High",0,50,1);//changed the row to 50 so that the temp. and humidity mesaages do not overlap
    delay(500);
    
  }
  else if (data.humidity < 60){
    digitalWrite(LED_3,HIGH);
    digitalWrite(LED_2,LOW);
    print_line("Humidity LOW",0,50,1);
    delay(500);
    
  }
}

void set_utc_offset() {
  int temp_offset_hours = 0; //  hours
  int temp_offset_minutes = 0; // minutes
  display.clearDisplay();  
  
  while (true) {
    print_line("Enter UTC offset:", 0, 0, 2);
    display.setCursor(0, 40);
    display.print(temp_offset_hours);
    display.print(":");
    if (temp_offset_minutes < 10) {
      display.print("0");
    }
    display.print(temp_offset_minutes);
    display.display();

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      display.clearDisplay();
      temp_offset_minutes += 30; // Increment by 30 minutes to get the exact time zone
      if (temp_offset_minutes >= 60) {
        temp_offset_hours += 1;
        temp_offset_minutes -= 60;
      }
      if (temp_offset_hours > 14) {
        temp_offset_hours = 14;
        temp_offset_minutes = 0;
      }
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_offset_minutes -= 30; // Decrement by 30 minutes
      if (temp_offset_minutes < 0) {
        temp_offset_hours -= 1;
        temp_offset_minutes += 60;
      }
      if (temp_offset_hours < -12) {
        temp_offset_hours = -12;
        temp_offset_minutes = 0;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      utc_offset = temp_offset_hours * 3600 + temp_offset_minutes * 60; // Convert both hours and minutes to seconds
      configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);
      display.clearDisplay();
      print_line("UTC offset set", 0, 0, 2);
      delay(1000);
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}