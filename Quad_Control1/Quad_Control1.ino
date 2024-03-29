#include <Servo.h>
#include <Wire.h>
#include <ADXL345.h>

const int pin3 = 12;
const int pin4 = 11;
const int pin1 = 10;
const int pin2 = 9;

float GyroX,GyroY,GyroZ,GyroTemp,biasGyroX, biasGyroY, biasGyroZ, biasAccelX, biasAccelY, biasAccelZ,xAng,yAng,cycle,pitchGyro,rollGyro,pitch,roll,yaw,accX,accY,accZ,CMy,CMx,altCorrect;
int motor1,motor2,motor3,motor4,delayCount,switchCond;
int globalSpeed = 65;
int forward = 0;
int strafe = 0;

const float pi = 3.14159;
int aX,aY,aZ;
int g_offx = 0;
int g_offy = 0;
int g_offz = 0;
int totalGyroXValues = 0;
int totalGyroYValues = 0;
int totalGyroZValues = 0;
int i,mX,mY,mZ;

float timeStep = 0.02;
float pitchAccel = 0;
float rollAccel = 0;
float gyroVar = 0.1;
float deltaGyroVar = 0.1;
float accelVar = 5;
float Pxx = 0.1;
float Pvv = 0.1;
float Pxv = 0.1;
float kx,kv;
unsigned long clockNew,clockOld;

ADXL345 adxl;
#define ITG3200_Address 0x68
#define address 0x1E
#define cycleDelay 5
#define accConv 0.0039
#define ESC_SCALING 1
#define ESC_MIN 90
#define ESC_MAX 180
#define ROLL_OFFSET 5
#define PITCH_OFFSET 8
#define YAW_OFFSET 0

Servo ESC1;
Servo ESC2;
Servo ESC3;
Servo ESC4;

void setup() {
  Wire.begin(); 
  Serial.begin(115200);
  delay(100);
  initGyro(); //Setup Gyro
  initAcc();  //Setup Accelerometer
  initMag();  //Setup Magnetometer
  delay(25);
  GyroCalibrate(); //Factory Setup Routine
  GyroCalibrate2(); //Calibrates Gyro and/or other sensors
  delay(25);
  clockOld = 0.0;
  initAngles(); //Sets the Gyro Angles to initially match the accelerometers
  ARM_Sequence();
  switchCond = 0;
  delayCount = 0;
  
}

void loop() {
  kalman(); //Almighty Kalman Filter
  calcYaw(); //Tilt Compensated Compass Code
  motorUpdate();
  
  if (delayCount > 450){
    if (switchCond == 0){
    globalSpeed = 0;
    delayCount = 0;
    switchCond = 1;
    } else {
     globalSpeed = 65;
     delayCount = 0;
     switchCond = 0;
    }
  }
  delayCount += 1;
  
  /*
  Serial.print("Pitch: ");
  Serial.println(pitch);
  Serial.print("Roll: ");
  Serial.println(roll);
  Serial.print("Yaw: ");
  Serial.println(yaw);
  Serial.print("Motor 1: ");
  Serial.println(motor1);
  Serial.print("Motor 2: ");
  Serial.println(motor2);
  Serial.print("Motor 3: ");
  Serial.println(motor3);
  Serial.print("Motor 4: ");
  Serial.println(motor4);
  */
  
  /*
  Serial.print("Cycle: ");
  Serial.println(cycle,4);
  Serial.print("Count: ");
  Serial.println(delayCount);
  Serial.print("Switch: ");
  Serial.println(switchCond);
  Serial.print("Speed: ");
  Serial.println(globalSpeed);
  */
  
}

void initGyro() {
   Wire.beginTransmission(ITG3200_Address); 
   Wire.write(0x3E);  
   Wire.write(0x00);   
   Wire.endTransmission(); 
   
   Wire.beginTransmission(ITG3200_Address); 
   Wire.write(0x15);  
   Wire.write(0x07);   
   Wire.endTransmission(); 
   
   Wire.beginTransmission(ITG3200_Address); 
   Wire.write(0x16);  
   Wire.write(0x1E);   // +/- 2000 dgrs/sec, 1KHz, 1E, 19
   Wire.endTransmission(); 
   
   Wire.beginTransmission(ITG3200_Address); 
   Wire.write(0x17);  
   Wire.write(0x00);   
   Wire.endTransmission();    
}

void initMag(){
  Wire.beginTransmission(address); //open communication with HMC5883
  Wire.write(0x02); //select mode register
  Wire.write(0x00); //continuous measurement mode
  Wire.endTransmission();
}

void initAcc(){
  adxl.powerOn();
  adxl.setFreeFallThreshold(7);
  adxl.setFreeFallDuration(45);
}

void GyroCalibrate(){

 int tmpx = 0;
 int tmpy = 0;
 int tmpz = 0; 

 g_offx = 0;
 g_offy = 0;
 g_offz = 0;
 
 for (char i = 0;i<10;i++)
    {
    delay(10);  
    getGyro();
    tmpx += GyroX;
    tmpy += GyroY;
    tmpz += GyroZ; 
    }  
 g_offx = tmpx/10;
 g_offy = tmpy/10;
 g_offz = tmpz/10;
 
}

void GyroCalibrate2(){
  for (i = 0; i < 50; i += 1) {
    getGyro();
    getAcc();
    totalGyroXValues += GyroX;
    totalGyroYValues += GyroY;
    totalGyroZValues += GyroZ;
    //totalAccelXValues += aX;
    //totalAccelYValues += aY;
    //totalAccelZValues += aZ;
    delay(20);
  }
  biasGyroX = totalGyroXValues / 50;
  biasGyroY = totalGyroYValues / 50;
  biasGyroZ = totalGyroZValues / 50;
  //biasAccelX = totalAccelXValues / 50;
  //biasAccelY = totalAccelYValues / 50;
  //biasAccelZ = (totalAccelZValues / 50) - 256;
  biasAccelX = 1;
  biasAccelY = 1;
  biasAccelZ = 1;
}

void initAngles(){
  getAcc(); //Obtains Initial Angles; Quad must be motionless
  pitchGyro = -xAng; //Accounts for Angle Differences
  pitch = -xAng;
  rollGyro = yAng;
  roll = yAng;
}

void ARM_Sequence(){
  ESC1.attach(pin1);
  ESC2.attach(pin2);
  ESC3.attach(pin3);
  ESC4.attach(pin4);
  delay(20);
  
  ESC1.write(180);
  ESC2.write(180);
  ESC3.write(180);
  ESC4.write(180);
  delay(2000);
  
  ESC1.write(90);
  ESC2.write(90);
  ESC3.write(90);
  ESC4.write(90);
  delay(3000);
  //delay(1000);
}

void getGyro() {
  Wire.beginTransmission(ITG3200_Address); 
  Wire.write(0x1B);       
  Wire.endTransmission(); 
  
  Wire.beginTransmission(ITG3200_Address); 
  Wire.requestFrom(ITG3200_Address, 8);    // request 8 bytes from ITG3200
  
  int i = 0;
  byte buff[8];
  while(Wire.available())    
  { 
    buff[i] = Wire.read(); 
    i++;
  }
  Wire.endTransmission(); 
    
  GyroX = ((buff[4] << 8) | buff[5]) - g_offx;
  GyroY = ((buff[2] << 8) | buff[3]) - g_offy;
  GyroZ = ((buff[6] << 8) | buff[7]) - g_offz;
  GyroTemp = (buff[0] << 8) | buff[1]; // temperature 
 
  
}

void getAcc(){
  adxl.readAccel(&aX, &aY, &aZ);
  accX = aX * accConv;
  accY = aY * accConv;
  accZ = aZ * accConv;
  xAng = atan2(aX,aZ)*(180/pi) + PITCH_OFFSET;
  yAng = atan2(aY,aZ)*(180/pi) + ROLL_OFFSET;
}

void getMag(){
  //Tell the HMC5883 where to begin reading data
  Wire.beginTransmission(address);
  Wire.write(0x03); //select register 3, X MSB register
  Wire.endTransmission();
  
  //Read data from each axis, 2 registers per axis
  Wire.requestFrom(address, 6);
  if(6<=Wire.available()){
    mX = Wire.read()<<8; //X msb
    mX |= Wire.read(); //X lsb
    mZ = Wire.read()<<8; //Z msb
    mZ |= Wire.read(); //Z lsb
    mY = Wire.read()<<8; //Y msb
    mY |= Wire.read(); //Y lsb
  }
}

void kalman(){
  clockNew = millis(); //Cycle Timing Code
  cycle = (((clockNew - clockOld)*1.0)/1000.0);
  getGyro();
  getAcc();
  
  //Pitch Prediction Code
  pitchAccel = atan2(-accX,accZ)*(180.0/pi) + PITCH_OFFSET;
  pitchGyro = pitchGyro + ((GyroX - biasGyroX)/14.375)*cycle;
  pitch = pitch + ((GyroX - biasGyroX)/14.375)*cycle;
  
  //Roll Prediction Code
  rollAccel = atan2(accY,accZ) * 180.0 / pi + ROLL_OFFSET;
  rollGyro = rollGyro - ((-GyroY - biasGyroY) / 14.375) * cycle; 
  roll = roll - ((-GyroY - biasGyroY) / 14.375) * cycle;
  
  //Measurement Mode
  Pxx += cycle * (2 * Pxv + cycle * Pvv);
  Pxv += cycle * Pvv;
  Pxx += cycle * gyroVar;
  Pvv += cycle * deltaGyroVar;
  kx = Pxx * (1 / (Pxx + accelVar));
  kv = Pxv * (1 / (Pxx + accelVar));
  
  //Finish that Kalman Stuff
  pitch += (pitchAccel - pitch) * kx;
  roll += (rollAccel - roll) * kx;
  
  Pxx *= (1 - kx);
  Pxv *= (1 - kx);
  Pvv -= kv * Pxv;
  
  clockOld = millis();
  delay(cycleDelay);
}

void calcYaw(){
  getMag();
  CMx = mX * cos(radians(pitch)) + mZ * sin(radians(pitch)); //Adjusts mX reading
  CMy = mX * sin(radians(roll)) * sin(radians(pitch)) + mY * cos(radians(roll)) - mZ * sin(radians(roll)) * cos(radians(pitch)); //Adjusts mY Reading
  yaw = atan2(CMy,CMx);
  if (yaw < 0){yaw += 2*PI;}
  if (yaw > 2*PI) {yaw -= 2*PI;}
  yaw = yaw * (180/PI);
  if (yaw <= 360 && yaw > 180) {yaw -= 360;}
}

void motorUpdate(){
  /*
  
  
  */
  //altCorrect = 1/(.5*cos(radians(roll))+0.5*cos(radians(pitch)));
  altCorrect = 1;
  
  //Pitch Correction
  motor1 = int(globalSpeed * altCorrect + globalSpeed * altCorrect * 1.0 * ((pitch + forward)/90.0) + 90.0);
  motor2 = int(globalSpeed * altCorrect - globalSpeed * altCorrect * 1.0 * ((pitch + forward)/90.0) + 90.0);
  
  //Roll Correction
  motor3 = int(globalSpeed * altCorrect + globalSpeed * altCorrect * 1.0 * ((roll + strafe)/90.0) + 90.0);
  motor4 = int(globalSpeed * altCorrect - globalSpeed * altCorrect * 1.0 * ((roll + strafe)/90.0) + 90.0);
  
  //Yaw Correction
  //motor1 = motor1 * (1 + (yaw/180));
  //motor2 = motor2 * (1 + (yaw/180));
  //motor3 = motor3 * (1 - (yaw/180));
  //motor4 = motor4 * (1 - (yaw/180));
  
  //Correct Over-Saturation
  if (motor1 > 180) {motor1 = 180;}
  if (motor2 > 180) {motor2 = 180;}
  if (motor3 > 180) {motor3 = 180;}
  if (motor4 > 180) {motor4 = 180;}
  
  //Correct Under-Saturation
  if (motor1 < 90) {motor1 = 90;}
  if (motor2 < 90) {motor2 = 90;}
  if (motor3 < 90) {motor3 = 90;}
  if (motor4 < 90) {motor4 = 90;}
  
  //Update Motors
  ESC1.write(motor1);
  ESC2.write(motor2);
  ESC3.write(motor3);
  ESC4.write(motor4);
}

/*
void launch(int launchDuration, int launchTopSpeed, int launchHoverSpeed){
  if (launchState == 0){
    launchStartTime = millis();
    launchState = 1;
  } else {
    if ((
  }
  
}

*/
