#include <SoftwareSerial.h>
#include "LowPower.h"
//#include <avr/sleep.h>
#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68
#include "DHT.h"
#define dhtapin 6
#define dhtbpin 9
#define dhtcpin 10
#define dhtdpin 11
#define dhtepin 5
#define PPHLM 4//pocet polozek hlavniho menu
//#define dhttyp DHT22
#define PSENZORU 10
#define PROBUZENI 2 //pin kterym probouzi uzivatel Arduino
#define NAPAJENI A3


SoftwareSerial displej(8,12); 
//pin 8 diplej-> Arduino, pin 5 Arduino -> displej 
SoftwareSerial OpenLog(7, 8);
#define resetOpenLog 4

int poslm = 0; //posl. mereni v sekundach behu procesoru
byte i = 0; //i budu pouzivat ve vsech moznejch for smyckach
byte e = 0; //-||-
int interval; //interval - pocet minut
byte posun = 0;
byte k = PPHLM-1;
boolean st = 0; //stav tlacitka "dolu" (poznacim si ho abych poznal jestli se zmenil
boolean sp = 0; //stav tlacitka "zvolit"
boolean spa = 0; //stav tlacitka zpet
boolean sta = 0;//stav tlacitka nahoru
int uspi = 10; //po kolika vterinach necinnosti se ma Arduino uspat
unsigned long poslint = 0; //posledni interakce; pri zmacknuti tlacitka se nastavi na millis/1000
String prijempc;
void setup() { 
  pinMode(NAPAJENI, OUTPUT); //pin A0 ovládá tranzistor, který odpojuje napájení periferií při přechodu do režimu spánku
  digitalWrite(NAPAJENI, HIGH);
  displej.begin(9600);
  Serial.begin(9600);
  Wire.begin();
 
  delay(500);
  //OpenLog.listen();
  nastavOpenLog();
  delay(50);
  while(!Serial.availableForWrite());
  delay(20);
  poslinabidkudopc();
  delay(2000);
  OpenLog.print("cd ..\r");
  delay(100);
  kontrola_nastav();
  bilo();
  jas(21);
  displej.write("Vitejte.");
  kontrola_casy();
  olconfig();
  OpenLog.print("md mereni\r");
  cekaznak(100);
  
  bilo();
  displej.write(">Historie mereni");
  druhy();
  displej.write("Nastaveni");
  delay(2000);
  
}

void loop() {
  while(1){
    if(millis()/1000>=poslint+uspi)spani();
    if(posunujmenu())break;
    if(zvolit()){
      switch(posun){
        case 0:
          historie();
          break;
        case 1:
          nastaveni();
          break;
        case 2:
          meritted();
          break;
        case 3:
          stavsyst();
          break;
      }
    }
  }

  switch (posun){
        case 0:
          bilo();
          displej.write(">Historie mereni");
          druhy();
          displej.write("Nastaveni");
          break;
        case 1:         
          bilo();
          displej.write(">Nastaveni");
          druhy();
          displej.write("Merit ted");
          break;
        case 2:          
          bilo();
          displej.write(">Merit ted");
          druhy();
          displej.write("Stav systemu");          
          break;
        case 3:         
          bilo();
          displej.write(">Stav systemu");
          druhy();
          displej.write("Historie mereni");
          break;
    }


}

void bilo(){
  displej.write(254);
  //rikam ze chci posouvat kurzor
  displej.write(128);
  //rikam, ze ho chci posunout na zacatek prvniho radku
  displej.write("                "); //16mezer
  druhy();
  displej.write("                "); //16mezer
  
  displej.write(254);
  //rikam ze chci posouvat kurzor
  displej.write(128);
}
//presune kurzor u displeje na zacatek 1. radku
void prvni(){
  displej.write(254);
  displej.write(128);
}
//presune kurzor u displeje na zacatek 2. radku
void druhy(){
  displej.write(254);
  displej.write(192);
}
//nastavi jas; vstup: 0- 29
void jas(int vstup){
  displej.write(124);
  displej.write(128 + vstup%30);
  delay(2);
}

void nastavjas(){
  k = 29;
  bilo();
  displej.write("jas: ");
  displej.print(String(posun, DEC));
  displej.write("/29");
  while(1){
    while(1){
    if(posunujmenu())break;  
    if(zpet()){
      k=2;
      posun = 2;
      bilo();
      displej.print(">jas");
      druhy();
      displej.print("interval");
      return;
    }
    }
    bilo();
    jas(posun);
    displej.write("jas: ");
    displej.print(String(posun, DEC));
    displej.write("/29");
  }
}
/*tlacitko - pokud je mezi danym analogovym pinem a zemi
vetsi napeti jak 900/1024*5V vyhodi 1, jinak 0
Zapojeni: 10k odpor ma na jedne noze zapojenou zem, 
na druhe analogovy pin a pres tlacitko vcc
*/
boolean tlacitko(int apin){
  if(analogRead(apin)>900) return 1;
  else return 0;
}
/*pokud je stisknute tlacitko na stejnem pinu, pultlacitko vyhodivzdy nulu*/
boolean pultlac(int apin){
  if(analogRead(apin)<600 && analogRead(apin)>400){
    delay(1);
  if(analogRead(apin)<600 && analogRead(apin)>400) return 1;
  }
  return 0;
}

boolean posunujmenu(){
  boolean a = 0;
  if(st==0){
    if(tlacitko(0)==1){
      st=1;
      poslint = millis()/1000;
      if(posun!=k){ 
        posun++;
        a=1;
      }
      else{ 
        posun=0;
        a=1;
      }
    }
  }
  else{
    if(tlacitko(0)==0) st=0;
  }

  if(sta==0){
    if(tlacitko(1)==1){
      sta=1;
      poslint = millis()/1000;
      if(posun!=0){
        posun--;
        a=1;
      }
      else{
        posun=k;
        a=1;
      }
      }
  }
  else{
    if(tlacitko(1)==0) sta=0;
  }
  return a;
}


//pokud se stav "pultlacitka" na AO 
//zmenil z 0 na 1, vrati 1, jinak vrati 0
boolean zvolit (){
   if(sp==0){
    if(pultlac(0)==1){
      sp=1;
      poslint = millis()/1000;
      return 1;
      }
  }
  else{
    if(pultlac(0)==0) sp=0;
  }
  return 0;
}

//vrati jedna pokud bylo zmacknuto tlac. zpet
//neceka jenom zkontroluje jednorazove stav
boolean zpet (){
     if(spa==0){
    if(pultlac(1)==1){
      spa=1;
      poslint = millis()/1000;
      return 1;
      }
  }
  else{
    if(pultlac(1)==0) spa=0;
  }
  return 0;
}


void nastavOpenLog(void){
  pinMode(resetOpenLog, OUTPUT);
  OpenLog.begin(9600);
  char a;

  digitalWrite(resetOpenLog, LOW);
  delay(100);
  digitalWrite(resetOpenLog, HIGH);

   while(1) {
    if(OpenLog.available()){

      a = OpenLog.read();
      if(a == '>') break;
      if(a == '<'){
        gotoCommandMode();
        break;
      }
    }
  }
}

//posle OL 3x ascii26, cimz ho uvede do CommandModu a pocka na odpoved OL
void gotoCommandMode(void) {
  OpenLog.write(26);
  delay(10);
  OpenLog.write(26);
  delay(10);
  OpenLog.write(26);
  delay(10);
  while(1) {
    if(OpenLog.available())
      if(OpenLog.read() == '>') break;
  }
}

/*funkce maze prichozi znaky z OpenLogu, dokud
neprecte > nebo dokud nebezi po dobu cekani.
v pripade preteceni millis() muze cekat az dvojnasobne dlouho*/
void cekaznak (long cekani){
  unsigned long f = millis();

  while(f+cekani<=f){
    delay(cekani/10);
    f=millis();
  }
  
  while(1){
    if(OpenLog.available()){
      if(OpenLog.read()=='>') return;
    }

      if(millis()>f+cekani) return;
    }

}

//ceka, dokud neni OpenLog.available(), nebo dokud nevyprsi limit
void cekaol (long cekani){
  unsigned long f = millis();

  while(f+cekani<=f){
    delay(cekani/10);
    f=millis();
  }
  
  while(1){
    if(OpenLog.available())return;

      if(millis()>f+cekani) return;
    }


}

void historie(void){
  bilo();
  displej.write(">po dnech");
  druhy();
  displej.write("jednotlive casy");
  posun=0;
  k=2;
  while(1){
  while(1){
    if(posunujmenu()) break;
    if(zpet()){
      k=PPHLM-1;
      posun=0;
      bilo();
      displej.write(">Historie mereni");
      druhy();
      displej.write("Nastaveni");
      return;
    }
  }
  
  switch(posun){
    case 0:
      bilo();
      displej.write(">po dnech");
      druhy();
      displej.write("jednotlive casy");
      break;
    case 1:
      bilo();
      displej.write(">jednotlive casy");
      druhy();
      displej.write("po tydnech");
      break;
    case 2:
       bilo();
       displej.write(">po tydnech");
       druhy();
       displej.write("po dnech");
       break;
  }
  }
}

void nastaveni(void){
  bilo();
  displej.write(">interval");
  druhy();
  displej.write("senzory");
  posun=0;
  k=2;
  while(1){
    while(1){
      if(posunujmenu()) break;
      if(zpet()){
      k=PPHLM-1;
      posun=1;
      bilo();
      displej.write(">Nastaveni");
      druhy();
      displej.write("Merit ted");
      return;
      }
      if(zvolit()){
        switch(posun){
          case 0:
            nastavinterval();
            break;
          case 1:
            nastavsenzory();
            break;
          case 2:
            nastavjas();
        }
      }
    
    }
    switch(posun){
      case 0:
        bilo();
        displej.write(">interval");
        druhy();
        displej.write("senzory");
        break;
      case 1:
        bilo();
        displej.write(">senzory");
        druhy();
        displej.write("jas");
        break;
      case 2:
        bilo();
        displej.write(">jas");
        druhy();
        displej.write("interval");
        break;
    }
  }
}

/*fce vypise na displej vlhkost a teplotu namerenou 1. senzorem.
Pokud je k dispozici popis zobrazi ten. Na dalsi senzory by melo jit prepinat sipkama
Pri chozeni zpet a pri opetovnem spusteni nekdy nefunguji popisky, nevim proc*/
void meritted(void){
  String mezivysledek = "";
  mezivysledek.reserve(28);
  boolean cteradky = 1;
  delay(20);
  int pz = 0; //pocet zapojenych senzoru
  int piny[PSENZORU];
  int kody[PSENZORU];
  float h;
  float t;

  for(i=0;i<PSENZORU;i++){
    piny[i] = -2;
    kody[i] = -2;
  }

  OpenLog.print("read jmena.txt\r");
  cekaol(80);
  while(1){
    delay(12);
    if(isDigit(OpenLog.peek())) break;
    else OpenLog.read();
  }
  for(i=0;i<PSENZORU;i++){
    while(1){
      delay(50);//
      if(isDigit(OpenLog.peek())) mezivysledek += char(OpenLog.read());
      else break;
    }
    
    if(mezivysledek.toInt()){
      delay(50);//
      //Serial.print("Do piny pridavam:");
      //Serial.println(mezivysledek.toInt());
      piny[i] = mezivysledek.toInt();  
      
    }
    cekaol(90);
    delay(50);
    OpenLog.read();
    mezivysledek = ""; 
    while(1){      
      if(isDigit(OpenLog.peek())) mezivysledek += char(OpenLog.read());
      else break;
    }
    
    if(mezivysledek.toInt()){
      delay(50);//
      kody[i] = mezivysledek.toInt();  
    }
    //else Serial.print("-2.5\n");
    mezivysledek = "";
    if(OpenLog.read()!='\r'){
      Serial.println("BR 0");
      break;
    }
    else{
      OpenLog.read();
    }
    
  }
    
  //Serial.println("Vypisuju tabulky.");
  for(i=0;i<PSENZORU;i++){
    Serial.print(piny[i]);
    Serial.print(" ");
    Serial.println(kody[i]);
  }
  bilo();
  mezivysledek = "";

  for(i=0; i<PSENZORU; i++){
    if(piny[i]!=-2&&kody[i]!=-2) pz++;
    else break;
  }
  mezivysledek = "";
  /*smycka for nize ma zjistit, jestli je 
  ve slovniku oznaceni pro teplomer 
  s kodem i, pak ma vypsat teplotu 
  a vlhkost na displej pod slovnikovym 
  oznacenim pokud existuje jinak pod pinem */
  for(i=0;i<pz;i++){
    delay(12);
    OpenLog.flush();
    //delay(15);//tady kdyz dam dlouhej delay tak se to rozesere nevim proc
    OpenLog.print("read slovnik.txt\r");
    delay(12);
    while(cteradky){
      while(1){      
        cekaol(120);
        if(!OpenLog.available()){
          cteradky = 0;
          break;
        }
        if(isDigit(char(OpenLog.peek()))) break;
        OpenLog.read();
      }
      while(1){
        if(isDigit(char(OpenLog.peek()))) mezivysledek += char(OpenLog.read());
        else break;
      }
      if(mezivysledek.toInt()==kody[i])break;
      mezivysledek = "";
    }
    cteradky = 1;
    OpenLog.read();
    bilo();
    delay(20);
    cekaol(40);
    if(OpenLog.available())for(e=0; e<16; e++){
      Serial.println("popis je");
        //Serial.print("Mysicka varila kasicku.\n");
        Serial.println(OpenLog.peek());
        if(OpenLog.available()&&isPrintable(char(OpenLog.peek())))
        {
          delay(15);
          displej.print(char(OpenLog.read()));}
          
        //if(OpenLog.available()&&char(OpenLog.peek())!='\r'&&char(OpenLog.peek())!='\n')displej.print(char(OpenLog.read()));
        
        else break;
      }
    else{
      Serial.println("popis neni");
      displej.print("senzor pinu: ");
      displej.print(piny[i]);
    }
    delay(100);
    druhy();
    delay(50);
    displej.print(teplota(piny[i]));
    displej.print("*C ");
    //while(!displej.availableForWrite());
    delay(140);
    displej.print(vlhkost(piny[i]));
    displej.print('%');
    //OpenLog.flush();
    while(1){
      if(st==0){
        if(tlacitko(0)==1){
          Serial.print("dolu\n");
          st=1;
          if(i == pz-1) i = -1;
          break;
          }
      }
      else{
        if(tlacitko(0)==0) st=0;
      }
    
      if(sta==0){
        if(tlacitko(1)==1){
          Serial.print("nahoru\n");
          sta=1;
          if(i!=0) i = i-2;
          else i = pz-2;
          break;
          }
      }
      else{
        if(tlacitko(1)==0) sta=0;
      }
      if(zpet()){
        posun = 2;
        k = PPHLM-1;
        bilo();
        displej.write(">Merit ted");
        druhy();
        displej.write("Stav systemu");
        return;
      }
      //Serial.print("Kremilek\n");
  }

    
  }
  if(isnan(h) || isnan(t)){
    displej.print("chyba");
  }
    while(1){
    if(zpet()){
      posun = 2;
      k = PPHLM-1;
      bilo();
      displej.write(">Merit ted");
      druhy();
      displej.write("Stav systemu");
      return;
    }
  }
  
}

void stavsyst(void){
  bilo();
  displej.print(peknycas());
  while(1){
    if(zpet()){
      posun = 3;
      k = PPHLM-1;
      bilo();
      displej.write(">Stav systemu");
      druhy();
      displej.write("Historie mereni");
      return;
    }
  }
}

void nastavinterval(void){
  bilo();
  k=255;
  posun = interval-1;
  displej.write("interval: ");
  druhy();
  displej.print(interval);
  while(1){
    while(1){
      if(posunujmenu())break;  
      if(zpet()){
        k=2;
        posun = 0;
        bilo();
        displej.print(">interval");
        druhy();
        displej.print("senzory");
        OpenLog.print("write nastav.txt\r");
        cekaol(250);
        OpenLog.print("int ");
        OpenLog.print(interval);
        OpenLog.print(",");
        OpenLog.print("\r\r\r");
        cekaol(250);
        return;
      }
    }
    bilo();
    interval = posun+1;
    displej.write("interval: ");
    druhy();
    displej.print(String(posun+1, DEC));
  }
  
}

void nastavsenzory(void){
  
}

int chyba(long cekani){
  unsigned long f = millis();

  while(f+cekani<=f){
    delay(cekani/10);
    f=millis();
  }
  int x=0;
  while(1){
    
    if(OpenLog.available()){
      if(OpenLog.peek()=='!') x = 1;
      if(OpenLog.read()=='>') return x;
    }
      if(millis()>f+cekani) return 2;
  }
}

void cekazapis(long cekani){
  unsigned long f = millis();

  while(f+cekani<=f){
    delay(cekani/10);
    f=millis();
  }
  
  while(1){
    if(OpenLog.available()){
      if(OpenLog.read()=='<') return;
    }
      if(millis()>f+cekani) return;
    }

}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}



//vrci podle parametru  0 vteriny 1 minuty 2 hodiny ...
int cas(int parametr){
   byte misto;
   Wire.beginTransmission(DS3231_I2C_ADDRESS);
   Wire.write(parametr);
   Wire.endTransmission();
   Wire.requestFrom(DS3231_I2C_ADDRESS, 1);

   switch(parametr){
    case 0:
    misto =  bcdToDec(Wire.read() & 0x7f);
    break;
    case 2:
    misto = bcdToDec(Wire.read() & 0x3f);
    break;
    default:
    misto = bcdToDec(Wire.read());
    break;
  }
  return int(misto); 
}

String celycas (){
  String vysledek;
  vysledek.reserve(40);
  if (cas(6)>=10) vysledek += cas(6);
  else{
    vysledek+='0';
    vysledek+=cas(6);
  }
  if (cas(5)>=10) vysledek += cas(5);
  else{
    vysledek+='0';
    vysledek+=cas(5);
  }
  if (cas(4)>=10) vysledek += cas(4);
  else{
    vysledek+='0';
    vysledek+=cas(4);
  }
  if (cas(2)>=10) vysledek += cas(2);
  else{
    vysledek+='0';
    vysledek+=cas(2);
  }
  if (cas(1)>=10) vysledek += cas(1);
  else{
    vysledek+='0';
    vysledek+=cas(1);
  }
  if (cas(0)>=10) vysledek += cas(0);
  else{
    vysledek+='0';
    vysledek+=cas(0);
  }
  return vysledek;
}

//vypise cas ve formatu "dd.mm.rr hh:mm:vv"
String peknycas(){
  String vysledek;
  vysledek.reserve(27);
  vysledek += cas(4);
  
  vysledek += '.';
  if (cas(5)>=10) vysledek += cas(5);
  else{
    vysledek+='0';
    vysledek+=cas(5);
  }
  vysledek += '.';
  if (cas(6)>=10) vysledek += cas(6);
  else{
    vysledek+='0';
    vysledek+=cas(6);
  }
  vysledek += ' ';
  if (cas(2)>=10) vysledek += cas(2);
  else{
    vysledek+='0';
    vysledek+=cas(2);
  }
  vysledek += ':';
  if (cas(1)>=10) vysledek += cas(1);
  else{
    vysledek+='0';
    vysledek+=cas(1);
  }
 
  return vysledek;
}

/*fce olconfig prepise config soubor na OL na "9600,26,3,2,0,0,0"*/
void olconfig(){
  OpenLog.print("write config.txt 0\r");
  cekaol(900);
  OpenLog.print("9600,26,3,2,0,0,0");
  delay(20);
  OpenLog.print("\r");
  delay(20);
  OpenLog.print("\r");
  delay(20);
  cekaol(900);
  //Serial.println("ol:");
  //while(OpenLog.available())Serial.println(int(OpenLog.read()));  
}

//Zkopirovany nekde z vebu rekne volnou ram vypise do komplu
int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

float vlhkost(int pin){
  float f;
  DHT dht(pin, DHT22);
  dht.begin();
  delay(8);
  f = dht.readHumidity(); 
  return f; 
}

float teplota(int pin){
  DHT dht(pin, DHT22);
  dht.begin();
  delay(7);
  return dht.readTemperature();  
}

//fce nize proveri jestli existuje na SDkarte soubor nastav.txt
//pokud neexistuje, zalozi ho a zapise tam int 30,
void kontrola_nastav(){
  OpenLog.print("new nastav.txt\r");
  cekaznak(100);
  OpenLog.readString();
  OpenLog.print("read NASTAV.TXT\r");
  Serial.println(":D");
  OpenLog.setTimeout(400);
  String prijem = OpenLog.readString();
  Serial.print(prijem);
  //prijem.reserve(50);

  if(prijem.indexOf("int ")!=-1){
    String interv = "";
    for(int i=0; i !=-2; i++){
      if(isDigit(prijem[prijem.indexOf("int ")+4+i]))interv += prijem[prijem.indexOf("int ")+4+i];
      else break;
    }
    interval = interv.toInt();
    if(interval == 0){
      interval = 30;
    }
// je v cm
  }
  else{
    Serial.println("Tady");
    OpenLog.print("append NASTAV.TXT\r");
    OpenLog.print("int 30,");
    interval = 30;
    delay(80);
    gotoCommandMode();
  }
}

void kontrola_casy(){
  OpenLog.readString();
  OpenLog.print("new casy.txt\r");
  if (chyba(100) !=1){
    Serial.print("casy nejsou");
    OpenLog.print("append casy.txt\r");
    cekazapis(100);
    OpenLog.print("c");
    OpenLog.println(celycas());
    OpenLog.write(10);
    OpenLog.print("001\r\n");
    OpenLog.print("002\r\n");
    OpenLog.print("003\r\n");
    OpenLog.print("004\r\n");
    OpenLog.print("005\r\n");
    delay(10);
    OpenLog.flush();
    gotoCommandMode();
    OpenLog.print("rm jmena.txt\r");
    delay(2);
    OpenLog.flush();
    OpenLog.print("new jmena.txt\r");
    cekaznak(100);
    OpenLog.print("append jmena.txt\r");
    cekazapis(100);
    OpenLog.print(dhtapin);
    OpenLog.print(' ');
    OpenLog.print("001\r\n");
    OpenLog.print(dhtbpin);
    OpenLog.print(' ');
    OpenLog.print("002\r\n");
    OpenLog.print(dhtcpin);
    OpenLog.print(' ');
    OpenLog.print("003\r\n");
    OpenLog.print(dhtdpin);
    OpenLog.print(' ');
    OpenLog.print("004\r\n");
    OpenLog.print(dhtepin);
    OpenLog.print(' ');
    OpenLog.print("005\r\n");
    delay(10);
    OpenLog.flush();
    delay(20);
    gotoCommandMode();
  }
}
void zmeritzapsat(){
  Serial.println(":-)");
  String mezivysledek;
  mezivysledek.reserve(12);
  int piny[PSENZORU];
  for(i=0; i<PSENZORU; i++){
    piny[i] = -2;
  }
  OpenLog.print("read jmena.txt\r");
  cekaol(200);
  for(i=0; i<PSENZORU; i++){
    while(!isDigit(OpenLog.peek()))OpenLog.read();
    while(isDigit(OpenLog.peek())) mezivysledek+=char(OpenLog.read());
    Serial.println(mezivysledek);
    Serial.println(mezivysledek.toInt());
    piny[i] = mezivysledek.toInt();
    while(OpenLog.read()!='\n');
    mezivysledek = "";
    cekaol(40);
    Serial.print(OpenLog.peek());
    if(OpenLog.peek()==26||OpenLog.peek()==46){
      piny[i+1] = 250;
      break;
    }
  }
  mezivysledek ="";
  Serial.println("Pole pinu:");
  for(i=0; i< PSENZORU; i++) Serial.println(piny[i]);
  mezivysledek.reserve(17);
  mezivysledek = celycas();
  OpenLog.readString();
  Serial.print(mezivysledek);
  mezivysledek.remove(6);
  Serial.println(mezivysledek);
  OpenLog.print("cd mereni\r");
  OpenLog.readString();

  OpenLog.print("new m" + mezivysledek + ".txt\r");
  cekaol(200);
  OpenLog.readString();
  OpenLog.print("append m" + mezivysledek + ".txt\r");
  cekaol(200);
  mezivysledek=celycas();
  mezivysledek.remove(0,6);
  Serial.println(mezivysledek);
  OpenLog.print(mezivysledek);
  for(i=0; i< PSENZORU; i++){
    if(piny[i]==250) break;
    OpenLog.print(" ");
    OpenLog.print(vlhkost(piny[i]));
    delay(250);
    OpenLog.print(" ");
    OpenLog.print(teplota(piny[i]));
    delay(250);
  }
  OpenLog.print("\r\n");
  cekaol(200);
  gotoCommandMode();
  delay(80);
  OpenLog.print("cd ..\r");
  delay(50);
  cekaol(200);
  OpenLog.print("ls\r");
  cekaol(200);
  poslm = millis()/1000;
  Serial.println("B-)");
}

boolean spani(){
  boolean manualne = 0;
  pinMode(PROBUZENI, INPUT);
  bilo();
  displej.print("Spim");
  delay(20);
  int doba = poslm + interval*60 - millis()/1000;
  if(doba <=0){
    zmeritzapsat();
    int doba = poslm + interval*60 - millis()/1000;
  }
  int psc = doba/8; //pocet pruchodu spaciho cyklu
  //attachInterrupt(digitalPinToInterrupt(PROBUZENI), isr, RISING); 

  digitalWrite(NAPAJENI, LOW);
  for(i=0; i<psc; i++){
    
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_ON);
    if(digitalRead(PROBUZENI)==1){
      manualne = 1;
      break;
    }
  }
  digitalWrite(NAPAJENI, HIGH);
  Serial.print("Jsem vzhuru.\n");
  //void rozsvitzhasni();
  //detachInterrupt(digitalPinToInterrupt(PROBUZENI));
  if(manualne==0)zmeritzapsat();
  else poslint = millis()/1000;
}
/*
void isr(){
  poslint = millis()/1000;
  Serial.println(";)");
}

void rozsvitzhasni(){
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(20000);
  digitalWrite(13, LOW);
}
*/
void poslinabidkudopc(){

OpenLog.print("cd mereni\r");
cekaol(200);
delay(40);
OpenLog.print("ls\r");
delay(110);
OpenLog.flush();
delay(80);
olpc();

}

void olpc(){
  cekaol(200);
  delay(6);
  while(OpenLog.available()){
    Serial.print(char(OpenLog.read()));
    
  }
}

void poslipcsoubor(){
  String mezivysledek = "";
  while(1){
    if(mezivysledek = "konec") break;
    OpenLog.print(Serial.readString());
  }
}

void vypisprijem(){
  String mezivysledek;
  mezivysledek = Serial.readString();
  Serial.print("prijem: ");
  Serial.print(mezivysledek);
}
void poslidata(){
  String mezivysledek;
  int polozka = 0;
  OpenLog.print("cd mereni\r");
  delay(50);
  while(1){
    OpenLog.print("ls\r");
    delay(50);
    while(1){
      if(OpenLog.peek()==62) return;
      if(OpenLog.available())if(char(OpenLog.peek())!='M') OpenLog.read();
      
    }
    for(i=polozka; i>0; i--){
      OpenLog.read();
      while(1){
        if(OpenLog.peek()==62) return;
        if(OpenLog.available())if(char(OpenLog.peek())!='M') OpenLog.read();
      }
    }
    while(char(OpenLog.peek())!=' ') mezivysledek+= char(OpenLog.read());
    OpenLog.print("read ");
    OpenLog.print(mezivysledek);
    OpenLog.print("\r");
    delay(50);
    Serial.println(mezivysledek);
    while(OpenLog.available()){
      Serial.print(char(OpenLog.read()));
      delay(1);
    }
  }
}

void nastartuj(){
  nastavOpenLog();
  displej.begin(9600);
  Serial.begin(9600);
  Wire.begin();
  delay(500); //čeká na displej
}


