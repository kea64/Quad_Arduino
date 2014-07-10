//Sharp IR Distance Sensor Read Code

int IRRaw;
int IRDist;
#define IRPin 0
#define IRCalc1 0.3012
#define IRCalc2 -1.1331

void setup() {
  Serial.begin(115200);
  
  

}

void loop() {
  IRRaw = analogRead(IRPin);
  //IRDist = int(1.0 * IRRaw ^ 1.0);
  IRDist = 2 ^ 2;
  Serial.println(IRDist);
  delay(50);
}
