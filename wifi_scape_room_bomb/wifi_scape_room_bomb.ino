#include <LiquidCrystal_I2C.h>

#include "arduino_secrets.h"
//Libraries
#include <SimpleKeypad.h>
#include <Password.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>


// Pantalla LCD I2C 20x4
#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif
// Custom characters
uint8_t bell[8]  = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
uint8_t note[8]  = {0x2,0x3,0x2,0xe,0x1e,0xc,0x0};
uint8_t clock[8] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0};
uint8_t heart[8] = {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};
uint8_t duck[8]  = {0x0,0xc,0x1d,0xf,0xf,0x6,0x0};
uint8_t check[8] = {0x0,0x1,0x3,0x16,0x1c,0x8,0x0};
uint8_t cross[8] = {0x0,0x1b,0xe,0x4,0xe,0x1b,0x0};
uint8_t retarrow[8] = {	0x1,0x1,0x5,0x9,0x1f,0x8,0x4};
byte bomb[8] = {
  0b00011,
  0b00100,
  0b00100,
  0b01110,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
};

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 20 chars and 4 line display

// STEPPER MOTOR 28BYJ-48 with ULN2003
// Define stepper motor driver pinout
#define IN1  2
#define IN2  3
#define IN3  4
#define IN4  5

// Secuencia de pasos (par máximo)
int paso [4][4] ={{1, 1, 0, 0},{0, 1, 1, 0},{0, 0, 1, 1},{1, 0, 0, 1}};

// WIFI Setup
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "YOUR WIFI SSID";        // your network SSID (name)
char pass[] = "YOUR WIFI PASS";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
WiFiServer server(80);

//Matrix Keypad Setup (Column 4 will not be connected [A,B,C,D])
const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {{'1','2','3'},{'4','5','6'},{'7','8','9'},{'#','0','*'}};
byte rowPins[rows] = {6,7,8,9}; //connect to the row pinouts of the keypad
byte colPins[cols] = {10,11,12}; //connect to the column pinouts of the keypad

SimpleKeypad kp1((char*)keys, rowPins, colPins, rows, cols);   // New keypad called kp1

// Password setup
Password password = Password("4317"); //password to unlock box, can be changed

// Leds
#define led_blue 21
#define led_green 14
#define led_yellow 15
#define led_red 16

// Buzzer
#define buzzer 20

const int tone_boom = 200; 
const int tone_boom_duration = 5000;
const int tone_1min = 250;
const int tone_1min_duration = 200;
const int tone_1min_interval = 500;
const int tone_5min = 250;
const int tone_5min_duration = 200;
const int tone_5min_interval = 1000;
const int tone_15min = 250;
const int tone_15min_duration = 200;
const int tone_15min_interval = 5000;
const int tone_keypad = 300;
const int tone_keypad_duration = 200;

// Counter setup
int estado = 1; // 1: armado, 2:detonacion, 3: desactivando, 4: desactivado, 5: standby.

int milisegundos;
int segundos;
int minutos; 
int horas;

long milisegundos_pasados = 0;
long milisegundos_penalizados = 0;
long milisegundos_anadidos = 0;
long temporizador_inicial = 3600000; //Milisegundos iniciales 60 min
long temporizador=temporizador_inicial;
long milisegundos_inicio_buzzer;
long milisegundos_final_buzzer=0;

void setup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();                 // swich on lcd backlight
  Serial.begin(9600);      // initialize serial communication
  lcd.createChar(1, check);
  lcd.createChar(2, cross);
  lcd.createChar(3, bomb);

  lcd.home();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Armando explosivo: ");
  delay(500);
  
  //displayKeyCodes();
  // Initialize Leds and Buzzer pins
  pinMode(led_blue, OUTPUT);
  pinMode(led_red, OUTPUT);
  pinMode(led_yellow, OUTPUT);
  pinMode(led_green, OUTPUT); 
  pinMode(buzzer, OUTPUT); 
  digitalWrite(led_red,HIGH);
  digitalWrite(led_green,HIGH);
  
  // Todos los pines del stepper en modo salida
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  abrirCompuerta();
  delay(2000);
  cerrarCompuerta();
  lcd.printByte(1);
  lcd.setCursor(0, 1);
  delay(500);
  lcd.print("Conectando WIFI:");
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    lcd.printByte(2);
    lcd.setCursor(0, 3);
    lcd.print("Fallo modulo WIFI");
    // don't continue ??? REVISAR
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);}
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
  lcd.setCursor(19, 1);
  lcd.printByte(1);
  delay(500);
  lcd.setCursor(0,2);
  lcd.print("Red: ");lcd.print(ssid);
  delay(500);
  
  digitalWrite(led_red,LOW);
  digitalWrite(led_green,LOW);
  lcd.setCursor(0,3);
  IPAddress ip = WiFi.localIP();
  lcd.print("IP: ");lcd.print(ip);
  delay(3000);
  lcd.clear();
}



void loop() {

switch (estado){
  case 1: { // bomba activada
    // Check if countdown is 0
    digitalWrite(led_red,HIGH);
    digitalWrite(led_green,LOW);
    if (temporizador<0){
      estado = 2;
    }
    // Bomb is armed and countdown > 0
    else{
      char key=kp1.getKey();
      if (key) {keyEvent(key);}

      // Calculate countdown
      milisegundos_pasados = millis();
      temporizador = temporizador_inicial - milisegundos_pasados - milisegundos_penalizados + milisegundos_anadidos;
      int segundos_restantes = temporizador/1000;
      int minutos_restantes = segundos_restantes/60;
      int horas_restantes = minutos_restantes/60;
      
      milisegundos = temporizador%1000;
      segundos = segundos_restantes%60;
      minutos = minutos_restantes%60;
      horas = horas_restantes;
      
      // Display the countdown
      lcd.setCursor(0,0);
      lcd.print("********************");
      lcd.setCursor(0,1);
      lcd.print("* Detonacion en... *");
      lcd.setCursor(0,2);
      char buf[12];
      sprintf(buf,"%02d:%02d:%02d:%02d",horas,minutos,segundos,milisegundos);
      lcd.print("*   ");lcd.print(buf);lcd.print("   *");;
      lcd.setCursor(0,3);
      lcd.print("********************");
      
      if (temporizador<60000){
        if (milisegundos_final_buzzer< millis()){
          milisegundos_inicio_buzzer=millis();
          milisegundos_final_buzzer=milisegundos_inicio_buzzer+tone_1min_interval;
          tone(buzzer,tone_1min,tone_1min_duration);
        }
      }
      
      if (temporizador< 300000 && temporizador> 60000){
        if (milisegundos_final_buzzer< millis()){
          milisegundos_inicio_buzzer=millis();
          milisegundos_final_buzzer=milisegundos_inicio_buzzer+tone_5min_interval;
          tone(buzzer,tone_5min,tone_5min_duration);
        }
      }

      if ( temporizador>300000 && temporizador<900000){
        if (milisegundos_final_buzzer < millis()){
          milisegundos_inicio_buzzer=millis();
          milisegundos_final_buzzer=milisegundos_inicio_buzzer+tone_15min_interval;
          tone(buzzer,tone_15min,tone_15min_duration);
        }
      }
    }
  break;
  }
  
  case 2: {// Detonación

  // LCD DISPLAY BOOM
  digitalWrite(led_red,HIGH);
  digitalWrite(led_yellow,HIGH);
  digitalWrite(led_green,HIGH);
  lcd.setCursor(0,0);
  lcd.print("********************");
  lcd.setCursor(0,1);
  lcd.print("*  BOOOOOOOOM!!!!  *");
  lcd.setCursor(0,2);
  lcd.print("*   00:00:00:000   *");
  lcd.setCursor(0,3);
  lcd.print("********************");
  tone(buzzer,tone_boom,tone_boom_duration);
  estado=5;
  break;
  }

  case 3: { // Bomb disarming
    // DISPLAY DISARMED
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("********************");
    lcd.setCursor(0,1);
    lcd.print("*   Desarmando...  *");
    lcd.setCursor(0,2);
    char buf[12];
    sprintf(buf,"%02d:%02d:%02d:%02d",horas,minutos,segundos,milisegundos);
    lcd.print("*   ");lcd.print(buf);lcd.print("   *");
    lcd.setCursor(0,3);
    lcd.print("********************");
    digitalWrite(led_red,LOW);
    digitalWrite(led_yellow,HIGH);
    digitalWrite(led_blue,HIGH);
    abrirCompuerta();
    delay(1000);
    tone(buzzer,550,1000);
    estado=4;    
    break;
    lcd.clear();
  }
  
  case 4: { // Bomb disarmed 
    // DISPLAY DISARMED
    lcd.setCursor(0,0);
    lcd.print("********************");
    lcd.setCursor(0,1);
    lcd.print("* Bomba desarmada  *");
    lcd.setCursor(0,2);
    char buf[12];
    sprintf(buf,"%02d:%02d:%02d:%02d",horas,minutos,segundos,milisegundos);
    lcd.print("*   ");lcd.print(buf);lcd.print("   *");
    lcd.setCursor(0,3);
    lcd.print("********************");
    digitalWrite(led_green,HIGH);
    digitalWrite(led_yellow,LOW);
    break;
  }
  case 5: {//standby after boom
    break;
    }
 }
  // Wifi Requests checking
  requestChecking();
}

void printWifiStatus() {

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void requestChecking() {
    WiFiClient client = server.available();   // listen for incoming clients
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            // the content of the HTTP response follows the header:
            client.print("<style>.button {display: block;width: 200px;height: 20px;background: #4E9CAF;padding: 10px;text-align: center;border-radius: 5px;color: white;font-weight: bold;line-height: 25px;} a:link {text-decoration: none;}a:visited {text-decoration: none;</style>}");
            client.print("<h1>Control manual de la WifiBombV2</h1><br>");
            client.print("<h2>Tiempo restante (HH:MM:SS) </h2><br>");
            client.print("<h2>");client.print(horas);client.print(":");client.print(minutos);client.print(":");client.print(segundos);client.print("</h2><br>");
            client.print("<a class='button' href='/'>Actualizar contador</a>");
            client.print("<h2>Modificar tiempos</h2><br>");
            //client.print("<a class='button' href='/restart'>Reiniciar contador</a><br>");
            client.print("<a class='button' href='/add/minute/1'>Sumar 1 minuto</a><br>");
            client.print("<a class='button' href='/add/minute/5'>Sumar 5 minutos</a><br>");
            client.print("<a class='button' href='/remove/minute/1'>Quitar 1 minuto</a><br>");
            client.print("<a class='button' href='/remove/minute/5'>Quitar 5 minutos</a><br>");
            client.print("<h2>Control de compuerta</h2><br>");
            client.print("<a class='button' href='/open'>Abrir compuerta</a><br>");
            client.print("<a class='button' href='/close'>Cerrar compuerta</a><br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        //if (currentLine.endsWith("GET /restart")) {estado=1;milisegundos_pasados = 0;milisegundos_penalizados = 0;milisegundos_anadidos = temporizador_inicial;}
        if (currentLine.endsWith("GET /add/minute/1")) {milisegundos_anadidos = milisegundos_anadidos+60000;}
        if (currentLine.endsWith("GET /add/minute/5")) {milisegundos_anadidos = milisegundos_anadidos+300000;}
        if (currentLine.endsWith("GET /remove/minute/1")) {milisegundos_anadidos = milisegundos_anadidos-60000;}
        if (currentLine.endsWith("GET /remove/minute/5")) {milisegundos_anadidos = milisegundos_anadidos-300000;}
        if (currentLine.endsWith("GET /open")) {abrirCompuerta();}
        if (currentLine.endsWith("GET /close")) {cerrarCompuerta();}
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
}

void keyEvent(char ekey) {
  Serial.println(ekey);
  tone(buzzer,tone_keypad,tone_keypad_duration);
  switch (ekey) {
    case '#': checkPassword(); delay(1); break;
    case '*': password.reset(); delay(1); tone(buzzer,250,400); break;
    default: password.append(ekey); delay(1);
   }
}

void checkPassword() {
  if (password.evaluate()) { //if password is right open box
    Serial.println("Accepted");
    tone(buzzer,500,400);
    estado = 3;
    delay(10);
  } else {
    Serial.println("Denied"); //if passwords wrong keep box locked
    tone(buzzer,150,400);
    milisegundos_penalizados= milisegundos_penalizados+temporizador/2; // La cagaste weon
    delay(10); //wait 0,3 seconds
    password.reset();
  }
}

void cerrarCompuerta(){
  for (int j = 0; j < 900; j++){   
    for (int i = 0; i < 4; i++)
    {digitalWrite(IN1, paso[i][0]);digitalWrite(IN2, paso[i][1]);digitalWrite(IN3, paso[i][2]);digitalWrite(IN4, paso[i][3]);delay(2);}
  }
}

void abrirCompuerta(){
  for (int j = 0; j < 900; j++){   
    for (int i = 0; i < 4; i++)
    {digitalWrite(IN4, paso[i][0]);digitalWrite(IN3, paso[i][1]);digitalWrite(IN2, paso[i][2]);digitalWrite(IN1, paso[i][3]);delay(2);}
  }
}
