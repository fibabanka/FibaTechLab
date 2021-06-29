#include <AFArray.h>
#include <DFRobot_sim808.h>
#include <SoftwareSerial.h>

#define PIN_TX    10
#define PIN_RX    11
SoftwareSerial mySerial(PIN_TX,PIN_RX);
DFRobot_SIM808 sim808(&mySerial);//Connect RX,TX,PWR,

AFArray<float> coordX;
AFArray<float> coordY;
AFArray<float> coordSpeed;
AFArray<String> coordDate;
char lat[12];
char lon[12];
char wspeed[12];
char body[400];
char cX[12];
char cY[12];
char cS[12];
String requestBody = "";
String http_cmd = "";
char buffer[100];
int delayed = 0;
int tryCount = 0;

void networkAvailable() {
  sim808Available();
  while(!sim808.join(F("internet"))) {
        Serial.println("Sim808 join network error");
        delay(100);
  }
  Serial.print("IP Address is ");
  Serial.println(sim808.getIPAddress());
}

void resetCoordinates() {
  coordX.reset();
  coordY.reset();
  coordSpeed.reset();
  coordDate.reset();
}

void addCord(float coordinateX, float coordinateY, float coordinateSpeed, String coordinateDate) {
    coordX.add(coordinateX);
    coordY.add(coordinateY);
    coordSpeed.add(coordinateSpeed);
    coordDate.add(coordinateDate);
}

void preapareSimOperations(){
  sim808Available();
}
void prepareGPSOperations(){
  sim808.detachGPS();
  if( sim808.attachGPS())
      Serial.println("Open the GPS power success");
  else 
      Serial.println("Open the GPS power failure");
}
void sim808Available() {
  sim808.close();
  sim808.disconnect();
  sim808.detachGPS();
  while(!sim808.init()) {
      Serial.print("Sim808 init error\r\n");
  }
  Serial.println("Sim808 init success");
  delay(500);
}

void getGpsCoordinate() {
  while(!sim808.getGPS())
  {

  }
  Serial.println("GPS verilerie ulaşıldı");
  float la = sim808.GPSdata.lat;
  float lo = sim808.GPSdata.lon;
  float ws = sim808.GPSdata.speed_kph;
  String month = "0" + String(sim808.GPSdata.month);
  String day = "0" + String(sim808.GPSdata.day);
  String hour = "0" + String(sim808.GPSdata.hour);
  String minute = "0" + String(sim808.GPSdata.minute);
  String second = "0" + String(sim808.GPSdata.second);
  String centisecond = "00" + String(sim808.GPSdata.centisecond);
  String gpsDate = String(sim808.GPSdata.year) +
  "-" +
  month.substring(month.length()-2, month.length()) +
  "-" +
  day.substring(day.length()-2, day.length()) +
  "T" +
  hour.substring(hour.length()-2, hour.length()) +
  ":" +
  minute.substring(minute.length()-2, minute.length()) +
  ":" +
  second.substring(second.length()-2, second.length()) +
  "." +
  centisecond.substring(centisecond.length()-3, centisecond.length()) +
  "Z";
  Serial.println(gpsDate);
  addCord(la, lo, ws, gpsDate);
  Serial.println("____________________________");
  dtostrf(coordX[0], 6, 6, cX);
  dtostrf(coordY[0], 6, 6, cY);
  dtostrf(coordSpeed[0], 6, 6, cS);
  //Serial.println(cX);
  //Serial.println(cY);
  //Serial.println(coordDate[0]);
}

void preapareHttpCmd() {
  requestBody = "[";
  for(int i = 0; i < coordX.size(); i++) {
    requestBody=requestBody+ "{\"x\":"+
    coordX[i]+",\"y\":"+
    coordY[i]+", \"vehicleId\": 144, \"gpsDate\": \""+
    coordDate[i]+"\", \"speed\":" + 
    coordSpeed[i] + "}";
    if(coordX.size() != i+1) requestBody+= + ",";
  }
  requestBody+="]";
  http_cmd = "";
  http_cmd = http_cmd + "POST /test-rabbit/coordinate-create-array HTTP/1.0\r\nContent-Type: application/json\r\nHost: 23.234.215.174\r\nContent-Length: "+ requestBody.length()+1 +"\r\n\r\n " + requestBody + " \r\n\r\n";
}

void sendCoordinates() {
      while(!sim808.connect(TCP,"23.234.215.174", 8080)) {
        Serial.println("Connect error");
        delay(500);
        tryCount++;
        if(tryCount > 15) networkAvailable();
      }
      tryCount = 0;
      Serial.println("Connect 23.234.215.174 success");

      char cmd[http_cmd.length()] = "";
      http_cmd.toCharArray(cmd, http_cmd.length());
      Serial.println(cmd);
      sim808.send(cmd, sizeof(cmd)-1);
      while (true) {
          int ret = sim808.recv(buffer, sizeof(buffer)-1);
          
          if (ret <= 0){
              Serial.println("fetch over...");
              Serial.println(buffer);
              break; 
          }
          buffer[ret] = '\0';
          Serial.print("Recv: ");
          Serial.print(ret);
          Serial.print(" bytes: ");
          Serial.println(buffer);
          break;
      }
      sim808.close();
      sim808.disconnect();
      Serial.println(http_cmd);
      http_cmd = "";
      resetCoordinates();
}

void setup(){
  mySerial.begin(9600);
  Serial.begin(9600);
  sim808Available();
  networkAvailable();
  prepareGPSOperations();
}

void loop(){

  getGpsCoordinate();
  if(delayed == 12000) {
    preapareHttpCmd();
    sendCoordinates();
    delayed = 0;
  }
  delay(10000);
  delayed+=3000;
}
