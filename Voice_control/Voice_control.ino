//Imports the BitVoicer library to the sketch
#include <BitVoicer11.h>
#include <SoftwareSerial.h>
#include <Stepper.h> 
SoftwareSerial mySerial(8,9); //Инициализируем софтварный порт на пинах 8 и 9
#define WIFI_SERIAL    mySerial

#define SPEED1      5 
#define DIR1        4
 
#define SPEED2      6
#define DIR2        7
BitVoicerSerial bvSerial = BitVoicerSerial(); //Создаем класс BitVoicer

byte dataType = 0;
byte pinVal = 0;  //мотор вкл/выкл
byte dir = 0;     //направление мотора

 
void setup() 
{
  pinMode   (4, OUTPUT);
  pinMode   (5, OUTPUT);
  pinMode   (6, OUTPUT);
  pinMode   (7, OUTPUT);
  Serial.begin(9600);  
  Serial.print("Serial init OK\r\n"); //выводим сообщение о том, что серийный порт открыт 
  // открываем Serial-соединение с Wi-Fi модулем на скорости 9600 бод
  WIFI_SERIAL.begin(9600);
  WIFI_SERIAL.println("AT+CWMODE=1"); //устанавливаем модуль в режим клиента 
      delay(1000);
  WIFI_SERIAL.println("AT+CWJAP_DEF=\"GoTo\",\"11110011\",\"f0:9f:c2:f1:07:a6\""); //подключаемся к wifi точке с заданным именем и паролем
      delay(1000);
       if (WIFI_SERIAL.find("OK")){
 Serial.print("WiFi connected \r\n");}
       else {Serial.print("ooops smth go wrong \r\n");}
  WIFI_SERIAL.println("AT+CIPSTA_DEF=\"172.6.0.200\",\"172.6.0.1\",\"255.255.255.0\""); //задаем IP адрес роутера, IP адрес точки доступа и маску подсети
      delay(1000);
        if (WIFI_SERIAL.find("OK")){
      Serial.print("IP address chandged successfully \r\n");}
        else {Serial.print("ooops smth go wrong \r\n");}
     
  WIFI_SERIAL.println("AT+CIPMUX=1");   //переключаем режим роутера на сервер
      delay(1000);
        if (WIFI_SERIAL.find("OK")){
      Serial.print("mode changed successfully \r\n");}
        else {Serial.print("ooops smth go wrong \r\n");}
  WIFI_SERIAL.println("AT+CIPSERVER=1,8080\r\n"); //создаем сервер на порту 8080
      delay(1000); 
        if (WIFI_SERIAL.find("OK")){
       Serial.print("Server created successfully \r\n");}
        else {Serial.print("ooops smth go wrong \r\n");}
}
void loop() 
{
   analogWrite(SPEED2, pinVal);  //включаем моторы
  digitalWrite(DIR2, dir);
    analogWrite(SPEED1, pinVal);
  digitalWrite(DIR1, dir);
    // если приходят данные из Wi-Fi модуля - отправим их в порт компьютера
  if (WIFI_SERIAL.available()) { 

     
  dataType = bvSerial.getData(); //читаем и сохраняем информацию, приходящую из буфера wifi модуля

  if (dataType == BV_BYTE) //проверяем, совпадает ли полученная информация с информацией в заданной голосовой схеме
  { 
    
    //смотрим, какая команда подходит под полученный сигнал 
    switch (bvSerial.byteData)
    {
    case 0: 
    
      pinVal = 255;
      dir=0;
      Serial.print("жалюзи опускаются\r\n");
      break;
   
    case 1: 
    
      pinVal = 255; 
      dir=1;
      Serial.print("жалюзи поднимаются \r\n");
      break;
    
    case 2: 
    
      pinVal = 0; 
      Serial.print("стоп\r\n"); 
      break;
      default: break;
    }
  }
  }
   //переносим информацию из серийного порта в софтварный WIFI порт
      if (Serial.available()) {
    WIFI_SERIAL.write(Serial.read());
  
   }    
}



