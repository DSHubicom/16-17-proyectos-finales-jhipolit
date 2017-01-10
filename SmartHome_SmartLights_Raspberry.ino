#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define PIR_GPIO 5
#define LED_BUILTIN 2
#define LED_A 4
#define LED_B 12
#define LED_C 13

/************************* Parámetros de conexión red WiFi WiFi *********************************/
#define WLAN_SSID       "nombreRed"
#define WLAN_PASS       "contrasena"

/************************* Parámatros de conexión Servidor MQTT (e.g., Adafruit.io) *********************************/
#define AIO_SERVER      "192.168.2.107"
#define AIO_SERVERPORT  1883                   //8883 para SSL

/************ Variables para cliente WiFi y cliente MQTT ******************/
// Crea un cliente ESP8266
WiFiClient client; //usar: WiFiClientSecure client; para cliente SSL

// Crea el cliente MQTT
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);

/****************************** Canales (Feeds) ***************************************/
Adafruit_MQTT_Publish canalPIR = Adafruit_MQTT_Publish(&mqtt, "casa/presencia");
Adafruit_MQTT_Publish canalLED1 = Adafruit_MQTT_Publish(&mqtt, "casa/luz1");
Adafruit_MQTT_Publish canalLED2 = Adafruit_MQTT_Publish(&mqtt, "casa/luz2");
Adafruit_MQTT_Publish canalLED3 = Adafruit_MQTT_Publish(&mqtt, "casa/luz3");


Adafruit_MQTT_Subscribe canalHORA = Adafruit_MQTT_Subscribe(&mqtt, "fecha/hora");

//Variable para el control de errores de envío al publicar
unsigned int errorPublicacion, hora;

char* cadenaP, estadoLED1, estadoLED2, estadoLED3;

/***************************************************************/
void setup() {
  Serial.begin(115200);

  //Configuramos los pines como entradas o salidas
  pinMode(PIR_GPIO, INPUT);
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(LED_C, OUTPUT);

  //Conectamos a la WiFi
  WIFI_connect();

  mqtt.subscribe(&canalHORA);
}


void loop() {
  // MQTT_connect() sirve tanto para la primera conexión como para en caso de producirse una desconexión, volver a conectar (por eso se llama en cada iteracción del loop)
  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &canalHORA) {
      hora=atoi((char*)canalHORA.lastread);
    }
  }
  Serial.println("La hora es: " + hora);
  int lecturaPIR=digitalRead(PIR_GPIO);
  if (lecturaPIR==0) {
    cadenaP="Casa vacía";
  }
  else {
    cadenaP="Hay usuarios en casa";
  }
  //****** PUBLICACION
  //Si no hay error de publicación, la función devuelve 0, sino el código de error correspondiente (sólo interesante para debug)
  if (! (errorPublicacion = canalPIR.publish(cadenaP))) {
    Serial.print("Error de publicación (error ");
    Serial.print(errorPublicacion);
    Serial.println(")");
  }
  else {
    switch (lecturaPIR) {
      case 0:
          digitalWrite(LED_A, LOW);
          digitalWrite(LED_B, LOW);
          digitalWrite(LED_C, LOW);
          canalLED1.publish("Apagado");
          canalLED2.publish("Apagado");
          canalLED3.publish("Apagado");
      break;
      case 1:
          if (hora>=15 && hora<18) {
          digitalWrite(LED_A, HIGH);
          digitalWrite(LED_B, LOW);
          digitalWrite(LED_C, LOW);
          canalLED1.publish("Encendido");
          canalLED2.publish("Apagado");
          canalLED3.publish("Apagado");

        }
        else if (hora>=18 && hora<=20)  {
          digitalWrite(LED_A, HIGH);
          digitalWrite(LED_B, HIGH);
          digitalWrite(LED_C, LOW);
          canalLED1.publish("Encendido");
          canalLED2.publish("Encendido");
          canalLED3.publish("Apagado");
        }
        else if (hora>20 && hora <=23) {
          digitalWrite(LED_A, HIGH);
          digitalWrite(LED_B, HIGH);
          digitalWrite(LED_C, HIGH);
          canalLED1.publish("Encendido");
          canalLED2.publish("Encendido");
          canalLED3.publish("Encendido");
        }
        else if (hora>=0 && hora <=1) {
          digitalWrite(LED_A, HIGH);
          digitalWrite(LED_B, HIGH);
          digitalWrite(LED_C, HIGH);
          canalLED1.publish("Encendido");
          canalLED2.publish("Encendido");
          canalLED3.publish("Encendido");
        }
        else {
          digitalWrite(LED_A, LOW);
          digitalWrite(LED_B, LOW);
          digitalWrite(LED_C, LOW);
          canalLED1.publish("Apagado");
          canalLED2.publish("Apagado");
          canalLED3.publish("Apagado");
        }
      break;
    }
  }
  delay(1000); //Publicamos cada segundo
}


// Funcción para conectar inicialmente y reconectar cuando se haya perdido la conexión al servidor MQTT
void MQTT_connect() {
  int8_t ret;

  if (!mqtt.connected()) {

    Serial.println("Conectando al servidor MQTT... ");

    uint8_t intentos = 3;
    while ((ret = mqtt.connect()) != 0) { // connect devuelve 0 cuando se ha conectado correctamente y el código de error correspondiente en caso contrario
      Serial.println(mqtt.connectErrorString(ret)); // (sólo interesante para debug)
      Serial.println("Reintentando dentro de 3 segundos...");
      mqtt.disconnect();
      delay(3000);  // esperar 3 segundos
      if (! intentos--) { //decrementamos el número de intentos hasta que sea cero
        while (1);  // El ESP8266 no soporta los while(1) sin código dentro, se resetea automáticamente, así que estamos forzando un reset
          //Si no quisieramos que se resetease, dentro del while(1) habría que incluir al menos una instrucción, por ejemplo delay(1); o yield();
      }
    }
    Serial.println("MQTT conectado");
  }
}

void WIFI_connect() {
  // Poner el módulo WiFi en modo station (este modo permite de serie el "light sleep" para consumir menos
  // y desconectar de cualquier red a la que pudiese estar previamente conectado
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);

  // Conectamos a la WiFi
  Serial.println("Conectando a la red WiFi");

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) { //Nos quedamos esperando hasta que conecte
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado.");
}

