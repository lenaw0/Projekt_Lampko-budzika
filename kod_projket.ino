#include <ESP8266WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>

const char* ssid = "";
const char* password = "";
WiFiServer server(80);// Set port to 80
String header; // This storees the HTTP request
const char* ntpServer = "pool.ntp.org";
const long utcOffsetInSeconds = 3600; // przesunięcie czasu dla twojej strefy czasowej
const int localTimeZone = 1; // przesunięcie czasowe dla trefy czasowej w stosunku do czasu UTC (czas letni)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds);

TimeChangeRule myRule = {Last, Sun, Mar, 2, 60};    // Dla czasu letniego
TimeChangeRule myRule2 = {Last, Sun, Oct, 2, 0};    // Dla czasu zimowego
Timezone myTimeZone(myRule, myRule2);

LiquidCrystal_I2C lcd(0x27, 16, 2); // Adres I2C wyświetlacza i rozmiar (16x2)

byte dzwonek[8] =
{
  0b00000,
  0b00100,
  0b01110,
  0b01110,
  0b01110,
  0b11111,
  0b00100,
  0b00000
};
const int hourButtonPin = 12; // Pin przycisku inkrementacji godziny alarmu
const int minuteButtonPin = 13; // Pin przycisku inkrementacji minut alarmu
const int confirmButtonPin = 2; // Pin przycisku potwierdzenia ustawień alarmu

int greenled = 15;
int blueled = 10;
int redled = 14;

String greenstate = "off";// state of green LED
String bluestate = "off";// state of green LED
String redstate = "off";// state of red LED
 
int alarmHour = 0;
int alarmMinute = 0;
bool alarmActive = false; // Zmienna informująca o stanie alarmu

void setup() {
pinMode(hourButtonPin, INPUT_PULLUP);
  pinMode(minuteButtonPin, INPUT_PULLUP);
  pinMode(confirmButtonPin, INPUT_PULLUP);

 pinMode(greenled, OUTPUT);
pinMode(blueled, OUTPUT);
 pinMode(redled, OUTPUT);

 digitalWrite(greenled, LOW);
digitalWrite(blueled, LOW);
 digitalWrite(redled, LOW);
 
  Serial.begin(115200);
pinMode(0, OUTPUT); // buzzer
  // Połączenie z WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Inicjalizacja wyświetlacza LCD
  lcd.init();
  lcd.backlight();

  // Pobranie aktualnego czasu z serwera NTP
  timeClient.begin();
  timeClient.update();

  
//analogWrite(red, 50);
//analogWrite(blue, 50);
//analogWrite(green, 50);

  lcd.createChar(0, dzwonek);
  // Print local IP address and start web server

 Serial.println("");

 Serial.println("WiFi connected.");

 Serial.println("IP address: ");

 Serial.println(WiFi.localIP());// this will display the Ip address of the Pi which should be entered into your browser

 server.begin();

}

void loop() {
  timeClient.update(); // Aktualizacja czasu

  // Pobranie czasu UTC
  time_t utc = timeClient.getEpochTime();

  // Konwersja czasu UTC na czas lokalny uwzględniający czas letni
  TimeChangeRule *tcr;
  time_t local = myTimeZone.toLocal(utc, &tcr);

  // Konwersja czasu lokalnego do tmElements_t
  tmElements_t localTime;
  breakTime(local, localTime);

  // Pobranie godziny, minut i sekund
  int hour = localTime.Hour;
  int minute = localTime.Minute;
  int second = localTime.Second;

  // Wyświetlenie godziny na wyświetlaczu LCD
  lcd.setCursor(0, 0);
  lcd.print("Godzina:");
  if (hour < 10) lcd.print("0");
  lcd.print(hour + 1);
  lcd.print(":");
  if (minute < 10) lcd.print("0");
  lcd.print(minute);
  lcd.print(":");
  if (second < 10) lcd.print("0");
  lcd.print(second);
  if (alarmActive){
  lcd.setCursor(0, 1);
  lcd.write(0);
  lcd.print(alarmHour);
  lcd.print(":");
  lcd.print(alarmMinute);
  }
  handleButtons();

  if (alarmActive && hour == alarmHour - 1 && minute == alarmMinute) {
    activateAlarm(); 
    Serial.println("elooooo");
    
  }

  
 WiFiClient client = server.available(); // Listen for incoming clients


 if (client) { // If a new client connects,

 String currentLine = ""; // make a String to hold incoming data from the client

 while (client.connected()) { // loop while the client's connected

 if (client.available()) { // if there's bytes to read from the client,

 char c = client.read(); // read a byte, then

 Serial.write(c); // print it out the serial monitor

 header += c;

 if (c == '\n') { // if the byte is a newline character

 // if the current line is blank, you got two newline characters in a row.

 // that's the end of the client HTTP request, so send a response:

 if (currentLine.length() == 0) {

 // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)

 // and a content-type so the client knows what's coming, then a blank line:

 client.println("HTTP/1.1 200 OK");

 client.println("Content-type:text/html");

 client.println("Connection: close");

 client.println();


 // turns the GPIOs on and off

 if (header.indexOf("GET /green/on") >= 0) {

 Serial.println("green on");

 greenstate = "on";

 digitalWrite(greenled, HIGH);

 } else if (header.indexOf("GET /green/off") >= 0) {

 Serial.println("green off");

 greenstate = "off";

 digitalWrite(greenled, LOW);

 } else if (header.indexOf("GET /red/on") >= 0) {

 Serial.println("red on");

 redstate = "on";

 digitalWrite(redled, HIGH);

 } else if (header.indexOf("GET /red/off") >= 0) {

 Serial.println("red off");

 redstate = "off";

 digitalWrite(redled, LOW);

 }
 else if (header.indexOf("GET /blue/on") >= 0) {

 Serial.println("blue on");

 bluestate = "on";

 digitalWrite(blueled, HIGH);

 } else if (header.indexOf("GET /blue/off") >= 0) {

 Serial.println("blue off");

 bluestate = "off";

 digitalWrite(blueled, LOW);

 }

 // Display the HTML web page

 client.println("<!DOCTYPE html><html>");

 client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

 client.println("<link rel=\"icon\" href=\"data:,\">");

 // CSS to style the on/off buttons

 // Feel free to change the background-color and font-size attributes to fit your preferences

 client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");

 client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");

 client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");

 client.println(".button2 {background-color: #77878A;}</style></head>");


 // Web Page Heading

 client.println("<body><h1>ESP8266 Web Server</h1>");


 // Display current state, and ON/OFF buttons for GPIO 5

 client.println("<p>green - State " + greenstate + "</p>");

 // If the green LED is off, it displays the ON button

 if (greenstate == "off") {

 client.println("<p><a href=\"/green/on\"><button class=\"button\">ON</button></a></p>");

 } else {

 client.println("<p><a href=\"/green/off\"><button class=\"button button2\">OFF</button></a></p>");

 }


 // Display current state, and ON/OFF buttons for GPIO 4

 client.println("<p>red - State " + redstate + "</p>");

 // If the red LED is off, it displays the ON button

 if (redstate == "off") {

 client.println("<p><a href=\"/red/on\"><button class=\"button\">ON</button></a></p>");

 } else {

 client.println("<p><a href=\"/red/off\"><button class=\"button button2\">OFF</button></a></p>");

 }
 // Display current state, and ON/OFF buttons for blue

 client.println("<p>blue - State " + bluestate + "</p>");

 // If the blue LED is off, it displays the ON button

 if (bluestate == "off") {

 client.println("<p><a href=\"/blue/on\"><button class=\"button\">ON</button></a></p>");

 } else {

 client.println("<p><a href=\"/blue/off\"><button class=\"button button2\">OFF</button></a></p>");

 }
 client.println("</body></html>");


 // The HTTP response ends with another blank line

 client.println();

 // Break out of the while loop

 break;

 } else { // if you got a newline, then clear currentLine

 currentLine = "";

 }

 } else if (c != '\r') { // if you got anything else but a carriage return character,

 currentLine += c; // add it to the end of the currentLine

 }

 }

 }

 // Clear the header variable

 header = "";

 // Close the connection

 //client.stop();

 Serial.println("Client disconnected.");

 Serial.println("");

 }
  delay(100); // Opóźnienie 1 sekundy
}

void handleButtons() {
  int hourButtonState = digitalRead(hourButtonPin);
  int minuteButtonState = digitalRead(minuteButtonPin);
  int confirmButtonState = digitalRead(confirmButtonPin);

  if (hourButtonState == LOW) {
    delay(100); // Opóźnienie dla eliminacji drgań
    alarmHour = (alarmHour + 1) % 24;
    Serial.print("Alarm Hour: ");
    Serial.println(alarmHour);
  }

  if (minuteButtonState == LOW) {
    delay(100); // 
    alarmMinute = (alarmMinute + 1) % 60;
    Serial.print("Alarm Minute: ");
    Serial.println(alarmMinute);
  }

  if (confirmButtonState == LOW) {
    delay(100); // 
    Serial.println("Alarm Set");
    alarmActive = true; // Ustawienie stanu alarmu na aktywny
  }
}

// Włączanie alarmu
void activateAlarm() {
 
 lcd.clear();
int aaa=1;

while(aaa==1){
   int confirmButtonState = digitalRead(confirmButtonPin);
  Serial.println("dzwiek wku");//test
   lcd.print("ALARM!");
   tone(0, 1000,3000); //
 delay(1000);
 tone(0, 2000,500); 
 delay(1000);
 tone(0, 200,5000);
  

  if (confirmButtonState == LOW) {
      noTone(0);
     lcd.clear();
    lcd.print("Milego Dnia ;]");
    delay(3000);
    alarmActive = false; // Wyłączenie alarmu po wyświetleniu komunikatu
     aaa= 0;
    handleButtons();
    Serial.println("jebany if");
  
  
  }
  //break;

  }
}
