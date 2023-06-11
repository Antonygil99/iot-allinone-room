#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>



//**************************************
//*********** MQTT CONFIG **************
//**************************************
const char *mqtt_server = "34.121.87.245";
const int mqtt_port = 1883;
const char *mqtt_user = "admin";
const char *mqtt_pass = "public";
const char *root_topic_subscribe = "topic/input";
const char *move_topic_subscribe = "topic/movement";
const char *sleep_topic_subscribe = "topic/sleep";
const char *root_topic_publish = "topic/output";
const char *temp_topic_publish = "topic/temperature";
const char *humd_topic_publish = "topic/humidity";


//**************************************
//*********** TIME ZONE ****************
//**************************************
#define NTP_OFFSET  -21600 // In seconds 
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "1.mx.pool.ntp.org"


//**************************************
//*********** SLEEP CONF ***************
//**************************************
#define DEEP_SLEEP_TIME 10

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
//**************************************
//*********** WIFICONFIG ***************
//**************************************
const char* ssid = "ARRIS-B8C2";
const char* password =  "AB537588D46344A4";


//const char* ssid = "Students";
//const char* password =  "P0l1t3cn1c4.b1s";


//**************************************
//*********** GLOBALES   ***************
//**************************************
WiFiClient espClient;
PubSubClient client(espClient);
char msg[25];
//DHT11 messages
char msg_humd[5];
char msg_temp[5];
char msg_rel1[2];
char msg_rel2[2];

long count=0;
long lastMsg = 0;

//Pulse rate 
const int PulsePin = 15;

//***********  RELAYS ******************
//*************PINES********************
const int RELAY1 = 27;
const int RELAY2 = 26;

//*********** SENSORS ******************
//*************PINES********************

const int PIR = 18;
const int DHTPin = 4;

#define DHTTYPE DHT11

//************** OLED ******************
//************* SCREEN******************
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//************************
//** F U N C I O N E S ***
//************************
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();
void printLocalTime();
void goToDeepSleep();

//************************
//** P U L S E R A T E ***
//******INTERRUPT*********
/*
volatile int BPM;
volatile int Signal;
volatile int IBI = 600;
volatile boolean Pulse = false;
volatile boolean QS = false;*/

//*************DHT11***********
DHT dht(DHTPin, DHTTYPE);

void setup() {

  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  

  pinMode(PulsePin, INPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(DHTPin,INPUT);
  pinMode(PIR,INPUT_PULLUP);

   display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

 timeClient.begin();
 dht.begin();
 //interruptSetup();
}

void loop() {
	
	if (!client.connected()) {
	  	client.subscribe(root_topic_subscribe);
		reconnect();
	}

    else  {
    String str = "La cuenta es -> " + String(count);
    str.toCharArray(msg,25);
    client.publish(root_topic_publish,msg);
    count++;
    delay(300);
  }
  client.loop();


	//Time zone
	display.clearDisplay();
  
	timeClient.update();
	String formattedTime = timeClient.getFormattedTime();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15,20);
  display.print(formattedTime);
  display.display(); // actually display all of the above


	//PIR SENSOR

    bool interPir = digitalRead(PIR);
	if(interPir){
    Serial.println("Presence detected");
	client.publish(move_topic_subscribe,"1");
	delay(200);
  }
  else {
	  client.publish(move_topic_subscribe,"0");
  }

	
	//read humidity
	float h = dht.readHumidity();
 	// Read temperature as Celsius (the default)
 	float t = dht.readTemperature();
	String str_h= String(h);
	String str_tem = String(t);

	

	Serial.print(h);
	Serial.print(t);

	str_h.toCharArray(msg_humd,5);
	str_tem.toCharArray(msg_temp,5);

	client.publish(temp_topic_publish,msg_temp);
	client.publish(humd_topic_publish,msg_humd);

	
   


	
	//sleep mode after 5 min

	
	
	
	

  
}




//*****************************
//***    CONEXION WIFI      ***
//*****************************
void setup_wifi(){
	delay(10);
	// Nos conectamos a nuestra red Wifi
	Serial.println();
	Serial.print("Conectando a ssid: ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("Conectado a red WiFi!");
	Serial.println("Dirección IP: ");
	Serial.println(WiFi.localIP());
}



//*****************************
//***    CONEXION MQTT      ***
//*****************************

void reconnect() {

	while (!client.connected()) {
		Serial.print("Intentando conexión Mqtt...");
		// Creamos un cliente ID
		String clientId = "ESP32_GIL";
		clientId += String(random(0xffff), HEX);
		// Intentamos conectar
		if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
			Serial.println("Conectado!");

      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(22,40);
      display.print("mqtt broker ");
      display.setCursor(20,40);
      display.print("conexion sucess");
      display.display(); // actually display all of the above

			client.subscribe(root_topic_subscribe);
  			 
			// Nos suscribimos
			if(client.subscribe(root_topic_subscribe)){
		
        Serial.println("Suscripcion ok");
		
      }else{
        Serial.println("fallo Suscripciión");
      }
		} else {
			Serial.print("falló :( con error -> ");
			Serial.print(client.state());
			Serial.println(" Intentamos de nuevo en 5 segundos");
			delay(5000);
		}
	}
}


//*****************************
//***       CALLBACK        ***
//*****************************

void callback(char* topic, byte* payload, unsigned int length){
	
char p[length + 1];
memcpy(p, payload, length);
p[length] = 0;

String msg(p);

    Serial.print("Changing output to ");
    if(msg == "1"){
      Serial.println("on");

      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(30,45);
      display.print("Ligth1 ON");

      display.display(); // actually display all of the above
        delay(800);
	  display.clearDisplay();
      digitalWrite(RELAY1, HIGH);
    }
    else if(msg == "0"){
      Serial.println("off");
      digitalWrite(RELAY1, LOW);
	    display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(30,45);
      display.print("Ligth1 OFF");

      display.display(); // actually display all of the above
        delay(800);
	  display.clearDisplay();
    }
	else if(msg == "2"){
      Serial.println("on");
      digitalWrite(RELAY2, HIGH);

	  display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(30,45);
      display.print("Ligth2 ON");

      display.display(); // actually display all of the above
        delay(800);
	  display.clearDisplay();
    }

	else if(msg == "3"){
      Serial.println("off");
      digitalWrite(RELAY2, LOW);

	 
	  display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(30,45);
      display.print("Ligth2 OFF");

      display.display(); // actually display all of the above
        delay(800);
	  display.clearDisplay();
    }

	else if(msg == "4"){
      Serial.println("Sleeping. . .");
      digitalWrite(RELAY2, LOW);

	 
	  display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(20,45);
      display.print("Sleeping . . .");

      display.display(); // actually display all of the above
        delay(1500);
	  display.clearDisplay();

		goToDeepSleep();
    }

  Serial.write(payload,length);
  Serial.println();

	

}

//*****************************
//***       SLEEP FUN       ***
//*****************************
void goToDeepSleep()
{
	esp_sleep_enable_timer_wakeup(15*60*1000000);
	esp_deep_sleep_start();
}

