#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>

const char* ssid     = "INFINITUM6F2C_2.4"; // Nombre de la red WiFi
const char* password = "Riit9qSwci"; // Contraseña de la red WiFi
const char* serverUrl = "https://apk-alerta.onrender.com/api/v1/gas-data"; // URL del servidor Node.js

#define sensor 32   // Pin analógico para el sensor de gas (P15)
#define buzzer 2    // Pin digital para el buzzer (P2)
#define lcd_sda 21  // Pin SDA de la LCD (P21)
#define lcd_scl 22  // Pin SCL de la LCD (P22)

bool alertaEnviada = false; // Variable para rastrear si se ha enviado la alerta

// Inicializa el display LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección I2C, 16 columnas y 2 filas

// Configuración de la conexión NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Definición de la cantidad de días por mes
const int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void setup() {
  Serial.begin(9600);
  Serial.println("\n");

  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Esperar a que se conecte a WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conexión WiFi establecida");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  lcd.init();
  lcd.backlight();
  pinMode(buzzer, OUTPUT);

  lcd.setCursor(1, 0);
  lcd.print("Cargando...");
  for (int a = 0; a <= 15; a++) {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(200);
  }
  lcd.clear();

  // Inicializa la conexión NTP
  timeClient.begin();
}

void loop() {
  // Actualiza la hora del cliente NTP
  timeClient.update();
  String hora = timeClient.getFormattedTime();

  // Obtiene la marca de tiempo Unix y la convierte en una fecha legible
  unsigned long epochTime = timeClient.getEpochTime();
  String fecha = epochToDateTime(epochTime);

  // Obtiene el valor del sensor de gas
  int gasValue = analogRead(sensor);
  gasValue = map(gasValue, 0, 4095, 0, 100); // Mapea el valor a un rango del 0 al 100

  // Mensajes de depuración
  Serial.print("Lectura del sensor de gas: ");
  Serial.println(gasValue);

  // Si el nivel de gas es mayor o igual al 50% y no se ha enviado la alerta, envía la alerta
  if (gasValue >= 50 && !alertaEnviada) {
    // Construye el cuerpo del JSON con los datos del sensor
    String jsonBody = "{\"cantidad_gas\": " + String(gasValue) + ", \"fecha\": \"" + fecha + "\", \"hora\": \"" + hora + "\"}";

    // Realiza una solicitud HTTP POST al servidor Node.js
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonBody);

    // Comprueba la respuesta del servidor
    if (httpResponseCode > 0) {
      Serial.print("Datos enviados correctamente. Código de respuesta HTTP: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error al enviar datos. Código HTTP: ");
      Serial.println(httpResponseCode);
    }

    // Libera los recursos
    http.end();

    // Establece la bandera de alerta enviada a verdadero
    alertaEnviada = true;
  }

  // Muestra el nivel de gas en el LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nivel de gas:");
  lcd.print(gasValue);
  lcd.print("%");

  // Si el nivel de gas es mayor o igual al 50%, activa el buzzer
  if (gasValue >= 50) {
    lcd.setCursor(0, 1);
    lcd.print("¡Alerta Gas!");
    digitalWrite(buzzer, HIGH);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Gas Normal");
    digitalWrite(buzzer, LOW);
    
    // Restablece la bandera de alerta enviada a falso para permitir futuras alertas
    alertaEnviada = false;
  }

  delay(1000); // Espera 1 segundo antes de realizar la siguiente lectura
}

// Función para convertir una marca de tiempo Unix en una fecha y hora legibles
String epochToDateTime(unsigned long epochTime) {
  const char * daysOfTheWeek[7] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};
  char dateBuffer[30]; // Ajusta el tamaño del búfer según la longitud máxima de la fecha y hora

  unsigned long epoch = epochTime;
  epoch += 3600 * -5; // Corregir zona horaria (UTC-5)

  int year;
  byte month, day, hour, minute, second;
  unsigned long days;
  int wday;

  second = epoch % 60;
  epoch /= 60;
  minute = epoch % 60;
  epoch /= 60;
  hour = epoch % 24;
  epoch /= 24;
  wday = ((epoch + 4) % 7) + 1;  // Dia de la semana 1:Lunes, 2:Martes, ... 7:Domingo

  year = 1970;
  days = 0;
  while ((unsigned)(days += (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365) <= epoch) {
    year++;
  }
  days -= (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
  epoch -= days;
  day = 0;
  while (true) {
    byte monthLength;
    monthLength = (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) ? 29 : monthDays[month]; // Modificado para usar directamente la variable monthDays
    if (epoch >= monthLength) {
      epoch -= monthLength;
      day++;
    } else {
      break;
    }
  }
  month = 0;
  while (epoch >= monthDays[month] + (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))) { // Modificado para usar directamente la variable monthDays
    epoch -= monthDays[month] + (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)));
    month++;
  }
  year += 1970;

  // Utiliza snprintf para evitar desbordamientos de búfer
  snprintf(dateBuffer, sizeof(dateBuffer), "%s %02d/%02d/%04d %02d:%02d:%02d", daysOfTheWeek[wday - 1], day, month + 1, year, hour, minute, second);

  return String(dateBuffer);
}

