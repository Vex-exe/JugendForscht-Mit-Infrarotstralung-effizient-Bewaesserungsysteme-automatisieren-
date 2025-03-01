#include <Stepper.h>
#include <Servo.h>
#define PI 3.1415926535897932384626433832795
  
  int i = 0;
  
  float NDMI; //Endziel, das ermittelt werden soll
  float minNDMI = -0.95;  //Minimal erlaubter NDMI-Wert, der zugelassen ist
  float Wert830;  //Wert der 830nm-Diode
  float Wert1600; //Wert der 1600-Diode
 
  //Stepper-Variablen
  int StepsPerRev = 400;  //Schritte pro 360-Grad-Umdrehung
  float GradProStep = 0.9;  //Winkelveränderung pro Schrizz
  int A_StepsPerRev = 2038;
  float A_Step = (2038/360);

  int Z_SP = 5; //Pins für den Z-Motor
  int Z_DP = 4;
  float Z_Rotation = 0; //Rotation auf der Z-Achse
  bool R_Rotation = true; //Dreht sich der Stepper nach rechts?

  int Y_SP = 3; //Pins für den Y-Motor
  int Y_DP = 2;
  const float Y_maxRotation = 70; //Maximale Rotation auf der Y-Achse
  float Y_Rotation = 70;  //Rotation auf der Y-Achse
  float Y_minRotation = 45;  //Minimale Rotation auf der Y-Achse
  bool Y_finished = false;  //Wurde die Rotation auf der Y-Achse vollständig durchgeführt?

  int A_SP = 6; //Pins für den A-Motor
  int A_DP = 7;

  //Trigonometrie-Variablen
  float Messradius;
  float xCoord;
  float yCoord;
  float Hoehe = 0.23;

  //Kommunikitation mit Python
  bool turnON = false;  //Soll die Messung laufen?
  String msg; //Signal von Python

  //Pins für die Servo-Motoren
  int Serv_Pin1 = 8;
  int Serv_Pin2 = 9;
  int Serv_Pin3 = 10;
  int Serv_Pin4 = 11;

  //Stepper-Motoren
  Stepper StepperZ = Stepper(StepsPerRev, Z_DP, Z_SP);
  Stepper StepperY = Stepper(StepsPerRev, Y_DP, Y_SP);
  //Stepper StepperA = Stepper(A_StepsPerRev, 8, 9, 10, 11);
  Stepper stepper(A_StepsPerRev, 8, 10, 9, 11);

  //Servo-Motoren
  Servo Servo1;
  Servo Servo2;
  Servo Servo3;
  Servo Servo4;

void setup() {
  Serial.begin(9600);
  StepperZ.setSpeed(100); //Legt die Geschwindigkeit der Stepper-Motoren fest
  StepperY.setSpeed(100);

  Servo1.attach(Serv_Pin1); //Weißt den Servo-Motoren ihre Pins zu
  Servo2.attach(Serv_Pin2);
  Servo3.attach(Serv_Pin3);
  Servo4.attach(Serv_Pin4);
}

void loop()
{
  if (Serial.available() > 0){  //Wenn ein Signal am Seria Port verfügbar ist, dann...
    msg = Serial.readString();  //Lese das Signal aus
    if(msg == "ON"){  //Ist das Signal "ON", dann...
        turnON = true;  //Lasse die Messung laufen
      }
    else if(msg == "OFF"){  //Ist das Signal "OFF", dann...
        turnON = false; //Stoppe die Messung
        reset();  //Setze die Motoren in ihre Ursprungsposition zurück
      }
    }
    if(turnON == true){ //Soll die Messung laufen, dann...
      stepRotate(); //Rotire die Stepper
      trigon(); //Berechne die Koordinaten des beobachteten Punktes
      calculate();  //Berechne den NDMI
      send(); //Sende die Werte an Python
      if(i == 5){
        automate(); //Bewässere Stellen, an denen der NDMI zu klein ist
        i = 0;
      }
      delay(100);
    }
}

void stepRotate(){

  //Bestimme, in welche Richtung sich der Motor drehen soll
  if(Z_Rotation >= 360){  //Hat der Z-Stepper 360 Grad erreicht, dann...
    R_Rotation = false; //Ändere die Richtung
  }
  else if(Z_Rotation <= -360){  //Hat der Z-Stepper -360 Grad erreicht, dann...
    R_Rotation = true;  //Ändere die Richtung
  }
  
  if(Z_Rotation < 360 && R_Rotation == true && Y_finished == true){ //Soll sich der Z-Stepper nach rechts drehen & Hat seine Rotation weniger als 360 Grad & Hat der Y-Stepper seine Rotation bereits durchgeführt, dann...
    StepperZ.step(-16); //Rotiere um einen Grad
    Z_Rotation = Z_Rotation + GradProStep;  //Aktualisiere die Z-Rotation
    Y_finished = false; //Lasse den Y-Stepper wieder rotieren
    i++;
  }
  else if(Z_Rotation > -360 && R_Rotation == false && Y_finished == true){  //Soll sich der Z-Stepper nach links drehen & Hat seine Rotation mehr als -360 Grad & Hat der Y-Stepper seine Rotation bereits durchgeführt, dann...
    StepperZ.step(16);  //Rotiere um einen Step
    Z_Rotation = Z_Rotation - GradProStep;  //Aktualisiere die Z-Rotation
    Y_finished = false; //Lasse die Y-Stepper wieder rotieren
    i++;
  }

  if(Y_Rotation > Y_minRotation && Y_finished == false){  //Ist der Y-Stepper noch nicht ferig rotiert, dann...
    StepperY.step(4);  //Rotiere um einen Grad
    Y_Rotation = Y_Rotation - 1;  //Aktualisiere die Y-Rotation
  }
  else if(Y_Rotation <= Y_minRotation && Y_finished == false){  //Ist der Y-Stepper noch nicht fertig rotiert, hat aber eine Rotation kleiner gleich als die minimale Rotation, dann...
    StepperY.step((Y_maxRotation- Y_Rotation) * -4);  //Rotiere zurück zur Ausgangsrotation
    Y_finished = true;  //Setze die Y-Rotation auf fertig
    Y_Rotation = Y_maxRotation; //Aktualisiere die Y-Rotation
  }
}

void trigon(){
  Messradius = Hoehe*tan(Y_Rotation*PI/180);  //Berechne den Messradius
  xCoord = Messradius*sin(Z_Rotation*PI/180); //Berechne die x-Koordinate
  yCoord = Messradius*cos(Z_Rotation*PI/180); //Berechne die y-Koordinate
}

void calculate(){
  Wert1600 = analogRead(4)*1.66; //Wert der 1600nm-Diode wird ausgelesen; Multipliziert, um den Filtereinfluss zu kürzen (Durchlässigkeit ca. 60%)
  Wert830 = analogRead(5)*1.54*1.37; //Wert der 830nm-Diode wird ausgelesen; Multipliziert, um den Filtereinfluss (Durchlässigkeit ca. 65%) und die Responsivität der Didode (ca.0.73) zu kürzen
  NDMI = (Wert830-Wert1600)/(Wert830+Wert1600); //Berechnung NDMI
}

void send(){
  //Gib die Daten im Serial Port aus. Diese werden vom Python-Programm ausgelesen
  Serial.print(xCoord);
  Serial.print(",");
  Serial.print(yCoord);
  Serial.print(",");
  Serial.print(NDMI);
  Serial.println(",");
}

void reset(){
  StepperZ.step(Z_Rotation* 16); //Lasse den Z-Motor zurück an seine Ursprungsposition laufen
  StepperY.step((Y_maxRotation- Y_Rotation) * -4);//Lasse den Y-Motor zurück an seine Ursprungsposition laufen
  Y_Rotation = Y_maxRotation; //Setze die Y-Rotation zurück 
  Z_Rotation = 0; //Setze die Z-Rotation zurück 
}

void automate(){
  if(NDMI < minNDMI){ //Ist der NDMI unter dem mindestens erlaubten Wert, dann...
    
    stepper.setSpeed(6);
    stepper.step(A_Step*Z_Rotation);
    if(Messradius <= 1.18){ //Prüfe, in welchem Abstand sich der Punkt befindet und führe die jeweilige if-Schleife aus
      Servo1.write(90); //Öffne das zugehörige Ventil...
      delay(2000);  //...für zwei Sekunden...
      Servo1.write(-90);  //und schließe es wieder
    }
    else if(Messradius <= 1.36 && Messradius > 1.18){
      Servo2.write(90);
      delay(2000);
      Servo2.write(-90);
    }
    else if(Messradius <= 1.55 && Messradius > 1.36){
      Servo3.write(90);
      delay(2000);
      Servo3.write(-90);
    }
    else if(Messradius > 1.55){
      Servo4.write(90);
      delay(2000);
      Servo4.write(-90);
    }
    stepper.setSpeed(6);
    stepper.step(-A_Step*Z_Rotation*1.1);
    delay(1000);
  }
}
