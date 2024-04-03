/*
  Fecha de creación: 03 de Abril de 2024
  por: Juan Camilo Alvarez Jaramillo

  El presente programa está basado en la fusión simpleReading de preparacionMateriales_13
  la diferencia radica en el uso del teclado matricial para seleccionar las opciones
  y digitar un código en el momento de la salida o devolución del material y en la presentación del id del lote 
  de material preparado para la escritura física en las bolsas.
  

*/

//#include <EEPROM.h>//https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM

#include "HX711.h" //This library can be obtained here http://librarymanager/All#Avia_HX711

#define LOADCELL_DOUT_PIN  16
#define LOADCELL_SCK_PIN  17

HX711 scale;

float calibration_factor = -288; //-7050 worked for my 440lb max scale setup

#define tareButton 35
String masa;

//en vez de botones defino la selección de opciones con el teclado

/*
#define entraButton 34
#define saleButton 39
#define desperdicioButton 36
*/

////incluyo librerías Y defino variables para el control del teclado
#include<Keypad.h>

const byte lineas = 4;
const byte columnas = 4;

char teclas [lineas][columnas] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};

byte pinesLineas[lineas] = {13, 12, 14, 27};
byte pinesColumnas[columnas] = {26, 25, 33, 32};

Keypad teclado = Keypad (makeKeymap (teclas), pinesLineas, pinesColumnas, lineas, columnas);


//incluyo librerías Y defino variables para el control del RFID
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN          21
#define RST_PIN         22
int ledPin = 5;
int ledWiFi = 4;
String datorfid;
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status; //variable to get card status

byte buffer[18];  //data transfer buffer (16+2 bytes data+CRC)
byte size = sizeof(buffer);
String dataL;
char dataChar[18];
String datoParaServidor;
byte longitudId = 6;

char cadena[2];
byte cadenaMolde = 3;
char idProduccion[16];
char idProduccion2[16];
char idP[16];
char id_Molde[4];

int cuentaRevision = 0;

int idProduccionMenor = 0;
int idProduccionMayor = 0;
String idProduccionString = "";
String idProduccionString2 = "";

uint8_t pageAddr = 0x06;  //In this example we will write/read 16 bytes (page 6,7,8 and 9).
//Ultraligth mem = 16 pages. 4 bytes per page.
//Pages 0 to 4 are for special functions.
String num_molde;
String cant_moldes;
String respuesta;
unsigned long nowtime = 0;



//DISPLAY LCD

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_SDA 15
#define I2C_SCL 2

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

//incluyo librerías y defino variables para el Webhost y la conexion WiFi.

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <WiFiMulti.h>
#include <stdio.h>
#include <Separador.h>


int address1 = 0;
int address2 = 30;
int contwifi = 0;

//WiFiManager wifiManager;

Separador separar;

int cuentaLecturas = 0;

int contadorSalida = 0;

String strhost = "trazabilidadmasterdent.online";

String strurl = "/control/interaccion_arduino.php?pre_php=";

char host[48];
int err;
int temp = 0;
float tempFloat;
float hum = 1;//este es el dato correspondiente a la estacion en la tabla3 del Webhost
String datos = "";
uint32_t chipID = 0;
String chipid = "";
float datatemp = 26;
float datahum = 42;
int ID = 1001;
String datagps = "6.169252;-75.590535";
String prueba = "prueba1001";
int P = 0;//variable presion
char str_val[6];
char str_final[16];
char str_code[8];//es posible que este dato no sea necesario con lectura real de RFID tag
unsigned long lastTime = 0;

byte proceso = 6; //proceso correspondiente a la actualización de la estación en la tabla rotulos2
int idMolde = 0;
byte casilla = 0;

String distancia = "";
String rotulo = "";
int claseTag = 0; //1=molde, 2=rotulo.

int descontados = 0;
int salida = 0;
int asignados = 0;



const byte pulsadorDerecha = 34;
const byte pulsadorIzquierda = 39;

WiFiClient client;

int num_respuesta = 0;
int num_respuesta2 = 3;

String respuesta9 = "";
String respId = "";
String respRef = "";


//const char* ssid = "PRODUCCION_MASTERDENT_EXT";
//const char* password = "M4ST3RD3NT2021.";

//const char* ssid = "Masterdent- wifi prod";
//const char* password = "Masterdent2022*";

const char* ssid = "CamiloAlvarez";
const char* password = "milo2004";

//const char* ssid = "PRODUCCION_MASTERDENT";
//const char* password = "M4ST3RD3NT2021.";


// función para iniciar la conexión WiFi

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");

  unsigned long timeout = millis();
  int j = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    lcd.setCursor(0, 0);
    lcd.print("Conectando WiFi");
    lcd.setCursor(j, 1);
    lcd.print(".");
    j++;
    digitalWrite(ledWiFi, HIGH);
    delay(100);
    digitalWrite(ledWiFi, LOW);
    delay(700);
    if (j == 16) {
      lcd.clear();
      j = 0;
    }
    if (millis() - timeout > 15000) {
      ESP.restart();
    }

  }
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
}

//////////////////////////////////////////////////////////////////////////////////////

void setup() {

  pinMode(ledPin, OUTPUT);
  pinMode(ledWiFi, OUTPUT);
  pinMode(tareButton, INPUT);
//  pinMode(entraButton, INPUT);
//  pinMode(saleButton, INPUT);
//  pinMode(desperdicioButton, INPUT);
  Serial.begin (115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  SPI.begin(); // inicio SPI bus

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("¡Bienvenido!");
  delay(1500);
  lcd.clear();

  //  //Init EEPROM
  //  EEPROM.begin(EEPROM_SIZE);
  //  leerEEPROM();
  //
  //  conectarWifi();

  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");



  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  scale.tare();  //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

  initWiFi();

  Serial.println("");
  Serial.println("WiFi connected");
  digitalWrite (ledWiFi, HIGH);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  Serial.println(F("Sketch has been started!"));

  lcd.clear();
  Serial.println("");
  Serial.println("WiFi connected");
  lcd.setCursor(0, 0);
  lcd.print("WiFi conectado");



  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());


  strhost.toCharArray(host, 49);

  for (int i = 0; i < 17; i = i + 8) {
    chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.print("Chip ID: "); Serial.println(chipID);
  chipid = String(chipID);
  delay(1000);


  Serial.println("Seleccione la tarea a realizar");
  Serial.println("presione 'A' ingresar material a la nevera");
  Serial.println("presione 'B' sacar material de la nevera");
  Serial.println("presione 'C' eliminar material endurecido");
  Serial.println("presione A y B para crear nuevos tag de colores");
  memcpy(buffer, " ", 16);
  bool status;



}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  //revisarWiFi();
  menuInicio();
  seleccionInicial();


}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void leerTag() {
  //
  //
  int lectura = 0;
  salida = 0;
  String datoTag;

  //si no hay un tag presente no haga nada.
  while (lectura == 0) {
    size = sizeof(buffer);

    //    if (digitalRead(tareButton == HIGH)) {
    //
    //      lectura++;
    //      cuentaRevision++;
    //      salida++;
    //
    //    }
    if ( mfrc522.PICC_IsNewCardPresent())
    {
      if ( mfrc522.PICC_ReadCardSerial())
      {
        //break;


        //a continuación se ejecuta leer
        // Read data ***************************************************
        Serial.println(F("leyendo datos ... "));
        //data in 4 block is readed at once.
        status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAddr, buffer, &size);
        if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Read() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
          return;
        }
        lcd.clear();
        Serial.print(F("Dato leído: "));
        lcd.setCursor(0, 0);
        lcd.print("Dato leido: ");
        //Dump a byte array to Serial
        for (byte i = 0; i < 10; i++) {
          Serial.write(buffer[i]);
          dataChar[i] = buffer[i];
          //codigoRFID+=String(char(buffer[i]));
          if (dataChar[i] != '\n') {//utilizo el \n para no rellenar el dato con el signo parecido a la E con ' ' se rellena
            lcd.setCursor(i, 1);
            lcd.print(dataChar[i]);
          }

          buffer[i] = '\0';


        }

        Serial.println();
        Serial.println("FIN DE LA LECTURA");

        digitalWrite(ledPin, HIGH);
        delay(700);
        digitalWrite(ledPin, LOW);
        lcd.clear();

        mfrc522.PICC_HaltA();//cierra la comunicación con el lector.
        mfrc522.PCD_StopCrypto1();//tomado del unificado de tarjeta común con 2 bloques.

        datoTag = String(dataChar);

        //después de que datoTag toma el valor de dataChar, vacío dataChar para que no conserve datos viejos

        for (byte i = 0; i < 10; i++) {
          dataChar[i] = '\0';
        }

        Serial.print("valor de datoTag=");
        Serial.println(datoTag);

        if (claseTag == 1) {
          distancia = datoTag;
        }
        else if (claseTag == 2) {
          rotulo = datoTag;
        }
        else {
          Serial.println("por favor defina la clase del tag leído");
        }

        Serial.print("valor de la variable distancia despues de la funcion char array to string = ");
        Serial.println (distancia);

        Serial.print("valor de la variable rotulo despues de la funcion char array to string = ");
        Serial.println (rotulo);
        lectura = 1;
      }
      return;
    }
    else {

      revisarWiFi();


      Serial.print("Reading: ");
      Serial.println(scale.get_units(), 0);
      //lcd.clear();

      lcd.setCursor(0, 1);
      //lcd.print(scale.get_units(), 0);
      masa = String(scale.get_units(), 0);
      lcd.print("   " + (String(scale.get_units(), 0)) + " gramos    ");
      //      Serial.print(" gramos"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
      //      Serial.print(" calibration_factor: ");
      //      Serial.print(calibration_factor);
      //      Serial.print(" Dato RFID");
      //      Serial.print(datoTag);
      //      Serial.print(" Variable masa: ");
      //      Serial.print(masa);
      //      Serial.println();
      if (digitalRead(tareButton) == HIGH) {
        scale.tare();
        unsigned long tareTime = millis();
        while (digitalRead(tareButton) == HIGH) {

          if (millis() - tareTime > 1200) {
            Serial.println(">>> menú principal !");
            //ESP.restart();
            //client.stop();
            //client.flush();
            contadorSalida++;
            salida++;
            return;
          }
        }

        //lcd.clear();
        //return;

      }

    }
  }
  //enviarDatos();
  //return datoTag;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void enviarDatos() {
  unsigned long timecontrol = millis();
  unsigned long deltatime = timecontrol - nowtime;
  //if (deltatime >= 5000)
  //{
  Serial.print("connecting to ");
  Serial.println(host);

  if (client.connect(host, 80)) {
    Serial.println("connected to server");

    //client.print("GET https://esp32sensoresiot.000webhostapp.com/control/conexion_arduino.php?pre_php="); // Enviamos los datos por GET
    client.print("GET https://trazabilidadmasterdent.online/control/interaccion_arduino.php?pre_php="); // Enviamos los datos por GET
    client.print(P, DEC);
    client.print("&hum_php=");
    client.print(hum, 2);
    client.print("&temp_php=");
    client.print(temp, DEC);
    client.print("&proceso_php=");
    client.print(proceso, DEC);
    client.print("&dist_php=");
    client.print(distancia);
    client.print("&rotulo_php=");
    client.print(rotulo);
    client.println(" HTTP/1.0");
    //client.println("Host: esp32sensoresiot.000webhostapp.com");
    client.println("Host: trazabilidadmasterdent.online");
    client.println("Connection: close");
    client.println();
    Serial.println("Envio con exito (al archivo controller/index y models/herramienta)");
    lcd.setCursor(0, 0);
    lcd.print("Envio Exitoso");

    //"https://esp32sensoresiot.000webhostapp.com/control/conexion_arduino.php?pre_php=10.2&hum_php=20.3&temp_php=30.1&dist_php=cerca" 7/usar en postman.com
  }

  Serial.println("% send to Server");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 7000) {
      Serial.println(">>> Client Timeout !");
      ESP.restart();
      client.stop();
      client.flush();
      return;
    }
  }
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.println(line);
    //if (line.length() < 250) {
    num_molde = separar.separa(line, ',', num_respuesta);
    cant_moldes = separar.separa(line, ',', num_respuesta2);
    respuesta = cant_moldes;
    Serial.println("codigo obtenido");
    Serial.println(num_molde);
    Serial.println("cantidad de moldes a asignar");
    Serial.println(cant_moldes);


    if (respuesta == "rotuloOK,") {

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Respuesta=");
      lcd.setCursor(0, 1);
      lcd.print("Ingreso Exitoso!");
      //      lcd.setCursor(0, 1);
      //      lcd.print(num_molde);
      digitalWrite(ledPin, LOW);
      delay(150);
      digitalWrite(ledPin, HIGH);
      delay(150);
      digitalWrite(ledPin, LOW);
      delay(150);
      digitalWrite(ledPin, HIGH);
      delay(150);
      digitalWrite(ledPin, LOW);
      delay(150);
      digitalWrite(ledPin, HIGH);
      delay(150);
      digitalWrite(ledPin, LOW);


    }
    else {
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print("Respuesta=");
//      lcd.setCursor(0, 1);
//      lcd.print("------->      ");
    }

    //}
    /*else {
      Serial.println("la línea de respuesta es demasiado grande= ERROR");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      ESP.restart();
      return;
      }
    */
  }

  //memset(buffer, 0, sizeof(buffer));
  memset(dataChar, 0, sizeof(dataChar));//vacío el dataChar

  if (num_molde.length() <= 10) { //si el dato recibido en la respuesta es de molde o rótulo lo guardo en dataChar.
    num_molde.toCharArray(dataChar, 18);
    Serial.println("longitud de num_molde menor a 10");
    Serial.println("dato num_molde guardado en dataChar");
  }
  else {
    respuesta9 = num_molde;
  }
  //memcpy(buffer, dataChar, 16); //guarda en el buffer el dato en arreglo. hacer esto cuando le presiono la b.
  Serial.println("wait 5 sec...");
  delay(500);//remplazar por el if de arriba.
  //distancia = "";
  //rotulo="";
  nowtime = timecontrol;
}


////////////////////////////////////////////////////////////////////////////////////



//******************************************************************

////////////////////////////////////////////////////////////////

//***************************************************************
//*******************************************************************+
void menuInicio () {
  lcd.setCursor(0, 0);
  lcd.print("A.Ingres B.Retir");
  lcd.setCursor(0, 1);
  lcd.print("   C.Desperd");
}
//***********************************************************************
/*
void seleccionInicial() {

  //TAREA: Modificar a lectura de teclado en vez de botones.

  contadorSalida = 0;
  if (digitalRead(entraButton) == HIGH || digitalRead(saleButton) == HIGH || digitalRead(desperdicioButton) == HIGH) {
    Serial.println("Ha seleccionado :  "  );

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ha seleccionado:");

    Serial.print(" = ");



    if (digitalRead(entraButton) == HIGH && digitalRead(saleButton) == LOW && digitalRead(desperdicioButton) == LOW) {




      while (contadorSalida == 0 ) {

        if (contadorSalida == 0) {

          Serial.println(" ingresar bolsas de material preparado a las neveras ");
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("ingresarMaterial");
          //          delay(400);
          //          lcd.clear();
          //          lcd.setCursor(0, 1);
          //          lcd.print("Peso= ");
          //          lcd.setCursor(0, 1);
          //          lcd.print("       Col= ");
          lcd.setCursor(0, 0);
          lcd.print("ingresarMaterial");
          Serial.println("ponga el material en la báscula y luego pase un imán con Tag por el lector");
          claseTag = 1;//por tratarse de un molde
          leerTag();
          if (salida == 1) {
            return;
          }

          delay(500);

          temp = String(masa).toInt();
          proceso = 11;
          hum = 8;
          num_respuesta = 0; //para obtener ingreso exitoso
          num_respuesta2 = 4;
          enviarDatos();
        }
        //        if (digitalRead(tareButton) == HIGH)  {
        //
        //          contadorSalida++;
        //
        //
        //        }
      }
    }


    else if (digitalRead(entraButton) == LOW && digitalRead(saleButton) == HIGH && digitalRead(desperdicioButton) == LOW) {




      while (contadorSalida == 0 ) {

        if (contadorSalida == 0) {

          Serial.println(" retirar bolsas de material preparado a las neveras ");
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("RetirarMaterial");
          //          delay(500);
          //          lcd.clear();
          //          lcd.setCursor(0, 1);
          //          lcd.print("Peso= ");
          //          lcd.setCursor(1, 1);
          //          lcd.print("Color= ");
          lcd.setCursor(0, 0);
          lcd.print("RetirarMaterial");
          claseTag = 1;//por tratarse de un molde
          leerTag();
          if (salida == 1) {
            return;
          }

          //          delay(500);
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("Digite # de");
          //          lcd.setCursor(0, 1);
          //          lcd.print("BolsasRetiradas:");
          //          delay(500);
          //          //descontar();
          temp = String(masa).toInt();
          proceso = 11;
          hum = 1;
          temp = -temp;
          num_respuesta = 0; //para obtener ingreso exitoso
          num_respuesta2 = 4;
          enviarDatos();
        }
        //        if (digitalRead(tareButton) == HIGH) {
        //
        //          contadorSalidaSalida++;
        //
        //
        //        }
      }
    }
    else if (digitalRead(entraButton) == LOW && digitalRead(saleButton) == LOW && digitalRead(desperdicioButton) == HIGH) {

      unsigned long despButtonTime = millis();
      while (digitalRead(desperdicioButton) == HIGH) {

        if (millis() - despButtonTime > 3000) {

          Serial.println(" grabar los nombres de los colores en los tags ");
          lcd.setCursor(4, 1);
          lcd.print("id Color");
          delay(500);
          lcd.clear();

          lcd.setCursor(0, 1);
          lcd.print("DesdeBaseDatos");
          delay(700);
          lcd.clear();



          //igualo el id menor al dato obtenido con el teclado

          idProduccionMenor = 1;

          rotulo = 1;

          Serial.println(rotulo);
          Serial.println(" cargar conjunto de referencias de rótulo, desde la base de datos, para grabarlas una por una en Tag  ");



          num_respuesta = 1; //igualo num_respuesta a 1 para obtener ultimo dato de la tabla_produc
          proceso = 10;
          enviarDatos();
          Serial.print("respuesta9 = ");
          Serial.println(respuesta9);

          //descompongo la respuesta 10 y la voy presentando en el display y grabandola en el tag
          //descomponerRespuestaMasiva();
          int k = 0;
          while (k < 1000) {
            byte returnTecla = 0;
            //separo el dato de id como par y el de referencia como impar
            respId = separar.separa(respuesta9, '*', k);
            Serial.print("id= ");
            Serial.println(respId);
            if (respId == "") {
              k = 1000;
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("¡Grabado");
              lcd.setCursor(4, 1);
              lcd.print("Finalizado!");
              delay(900);
            }
            else {
              respRef = separar.separa(respuesta9, '*', k + 1);
              Serial.print("color= ");
              Serial.println(respRef);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Id: ");
              lcd.setCursor(0, 1);
              lcd.print("Color: ");
              lcd.setCursor(4, 0);
              lcd.print(respId);
              lcd.setCursor(7, 1);
              lcd.print(respRef);
              //guardo el id en el dataChar para guardarlo en el tag.
              respRef.toCharArray(dataChar, 18);
              returnTecla = escribirRespuestaMasiva();
              delay(500);

              if (returnTecla == 2) {
                //miro si el valor mostrado de la lista es el primero, en tal caso no desciende más
                if (k == 0) {

                }
                else {
                  k -= 2;
                }
              }
              else if (returnTecla == 3) {
                k = 1000;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("X---->Salir");
                delay(500);
              }
              else {
                k += 2;
              }
            }
          }
          return;
          if (salida == 1) {
            return;
          }
        }
      }



      while (contadorSalida == 0 ) {

        if (contadorSalida == 0) {

          Serial.println(" Descartar material endurecido");
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("BotarMaterial");
          //          delay(400);
          //          lcd.clear();
          //          lcd.setCursor(0, 1);
          //          lcd.print("Peso= ");
          //          lcd.setCursor(1, 1);
          //          lcd.print("Color= ");
          lcd.setCursor(0, 0);
          lcd.print("EliminarMaterial");
          claseTag = 1;//por tratarse de un molde
          leerTag();
          if (salida == 1) {
            return;
          }

          //          delay(500);
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("Digite # de");
          //          lcd.setCursor(0, 1);
          //          lcd.print("BolsasIngresadas:");
          //          delay(500);
          //          //descontar();
          temp = String(masa).toInt();
          proceso = 11;
          hum = 9;
          num_respuesta = 0; //para obtener ingreso exitoso
          num_respuesta2 = 4;
          enviarDatos();
        }
        //        if (digitalRead(tareButton) == HIGH) {
        //
        //          contadorSalida++;
        //
        //
        //        }
      }
    }





  }
}
*/
//**********************************************************************
void seleccionInicial() {
  char teclaOprimida = teclado.getKey();
  if (teclaOprimida != NO_KEY) {
    Serial.println("Ha seleccionado :  "  );
    Serial.print (teclaOprimida);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ha seleccionado:");
   

    if (teclaOprimida == 'A') {
      Serial.println(" ingresar bolsas de material preparado a las neveras ");
      lcd.setCursor(0, 1);
      lcd.print("ingresarMaterial");
      Serial.println("ponga el material en la báscula y luego pase un imán con Tag por el lector");
          claseTag = 1;//por tratarse de un molde
          leerTag();
          if (salida == 1) {
            return;
          }

          delay(500);

          temp = String(masa).toInt();
          proceso = 11;
          hum = 8;
          num_respuesta = 0; //para obtener ingreso exitoso
          num_respuesta2 = 4;
          enviarDatos();
    }
  

    else if (teclaOprimida == 'B') {
      char tecla ;


      int contador = 0;
      while (contador <= 1 ) {
        tecla  = teclado.getKey();//ANTERIORMENTE ESTABA completo con definición y asignación ABAJO DEL WHILE contador sizeof

        if (contador == 0) {

          Serial.println(" Retirar material de la nevera ");
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("RetirarMaterial");
          claseTag = 1;//por tratarse de un molde
          leerTag();
          if (salida == 1) {
            return;
          }

          
          temp = String(masa).toInt();
          proceso = 11;
          hum = 1;
          temp = -temp;
          num_respuesta = 0; //para obtener ingreso exitoso
          num_respuesta2 = 4;
          enviarDatos();
        }
        
      }
    }

    else if (teclaOprimida == 'C') {
      Serial.println(" Lectura ");

     

          lcd.setCursor(0, 1);
          lcd.print("Grab Id Color");
          delay(500);
          lcd.clear();



          //igualo el id menor al dato obtenido con el teclado

          idProduccionMenor = 1;

          rotulo = 1;

          Serial.println(rotulo);
          Serial.println(" cargar conjunto de referencias de rótulo, desde la base de datos, para grabarlas una por una en Tag  ");



          num_respuesta = 1; //igualo num_respuesta a 1 para obtener ultimo dato de la tabla_produc
          proceso = 10;
          enviarDatos();
          Serial.print("respuesta9 = ");
          Serial.println(respuesta9);

          //descompongo la respuesta 10 y la voy presentando en el display y grabandola en el tag
          //descomponerRespuestaMasiva();
          int k = 0;
          while (k < 1000) {
            byte returnTecla = 0;
            //separo el dato de id como par y el de referencia como impar
            respId = separar.separa(respuesta9, '*', k);
            Serial.print("id= ");
            Serial.println(respId);
            if (respId == "") {
              k = 1000;
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("¡Grabado");
              lcd.setCursor(4, 1);
              lcd.print("Finalizado!");
              delay(900);
            }
            else {
              respRef = separar.separa(respuesta9, '*', k + 1);
              Serial.print("color= ");
              Serial.println(respRef);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Id: ");
              lcd.setCursor(0, 1);
              lcd.print("Color: ");
              lcd.setCursor(4, 0);
              lcd.print(respId);
              lcd.setCursor(7, 1);
              lcd.print(respRef);
              //guardo el id en el dataChar para guardarlo en el tag.
              respRef.toCharArray(dataChar, 18);
              returnTecla = escribirRespuestaMasiva();
              delay(500);

              if (returnTecla == 2) {
                //miro si el valor mostrado de la lista es el primero, en tal caso no desciende más
                if (k == 0) {

                }
                else {
                  k -= 2;
                }
              }
              else if (returnTecla == 3) {
                k = 1000;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("X---->Salir");
                delay(500);
              }
              else {
                k += 2;
              }
            }
          }
          return;
          if (salida == 1) {
            return;
          }
        
      



      while (contadorSalida == 0 ) {

        if (contadorSalida == 0) {

          Serial.println(" Descartar material endurecido");
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("BotarMaterial");
          //          delay(400);
          //          lcd.clear();
          //          lcd.setCursor(0, 1);
          //          lcd.print("Peso= ");
          //          lcd.setCursor(1, 1);
          //          lcd.print("Color= ");
          lcd.setCursor(0, 0);
          lcd.print("EliminarMaterial");
          claseTag = 1;//por tratarse de un molde
          leerTag();
          if (salida == 1) {
            return;
          }

          //          delay(500);
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("Digite # de");
          //          lcd.setCursor(0, 1);
          //          lcd.print("BolsasIngresadas:");
          //          delay(500);
          //          //descontar();
          temp = String(masa).toInt();
          proceso = 11;
          hum = 9;
          num_respuesta = 0; //para obtener ingreso exitoso
          num_respuesta2 = 4;
          enviarDatos();
        }
        //        if (digitalRead(tareButton) == HIGH) {
        //
        //          contadorSalida++;
        //
        //
        //        }
      }
    }

  
    else {
      Serial.println("incorrecta");
      lcd.setCursor(5, 1);
      lcd.print("incorrecta");
      delay(500);
      lcd.clear();

    }
  }
  }






//*************************************************************************

///////////////////

void revisarWiFi() {
  int j = 0;
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledWiFi, LOW);
    WiFi.begin(ssid, password);

    unsigned long timeout = millis();
    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(ledWiFi, HIGH);
      delay(100);
      digitalWrite(ledWiFi, LOW);
      delay(700);
      Serial.print(".");
      lcd.setCursor(0, 0);
      lcd.print("Conectando WiFi");
      lcd.setCursor(j, 1);
      lcd.print(".");
      j++;
      if (j == 16) {
        lcd.clear();
        j = 0;
      }
      if (millis() - timeout > 15000) {
        ESP.restart();
      }

    }
    Serial.println("");
    Serial.println("WiFi connected");
    digitalWrite (ledWiFi, HIGH);
  }
  else {
    digitalWrite(ledWiFi, HIGH);
    return;
  }
}

//**********************************************************************

byte escribirRespuestaMasiva() {

  /*

  // declaro la variable a retornar
  byte navegaRef = 0;
  //mientras no halla tarjeta todavía

  memcpy(buffer, dataChar, 16); //guarda en el buffer el dato en arreglo.


  //agrego variable de escritura para mantener el estado de alerta mientras no haya tarjeta
  //char tecla;
  int escritura = 0;

  //agrego mensaje para pasar tarjeta despues de recibir el dato en el terminal

  while (escritura == 0) {
    size = sizeof(buffer);


    if (digitalRead(entraButton) == HIGH || digitalRead(saleButton) == HIGH || digitalRead(desperdicioButton) == HIGH) {
      if (digitalRead(entraButton) == HIGH && digitalRead(saleButton) == LOW && digitalRead(desperdicioButton) == LOW) {
        for (byte i = 0; i < 16; i++) {
          buffer[i] = '\0';
          dataChar[i] = '\0';
        }
        navegaRef = 1; //valor de 1 para subir en la lista de referencias
        return navegaRef;
      }
      else if (digitalRead(entraButton) == LOW && digitalRead(saleButton) == HIGH && digitalRead(desperdicioButton) == LOW) {

        for (byte i = 0; i < 16; i++) {
          buffer[i] = '\0';
          dataChar[i] = '\0';
        }
        navegaRef = 2; //valor de 2 para bajar en la lista de referencias
        return navegaRef;
      }
      else if (digitalRead(entraButton) == LOW && digitalRead(saleButton) == LOW && digitalRead(desperdicioButton) == HIGH) {
        for (byte i = 0; i < 16; i++) {
          buffer[i] = '\0';
          dataChar[i] = '\0';
        }
        navegaRef = 3; //valor de 3 para salir de la lista de referencias
        return navegaRef;
      }
      else {

      }
    }
    Serial.println("pase una tarjeta por el lector");

    if (mfrc522.PICC_IsNewCardPresent()) {

      if ( mfrc522.PICC_ReadCardSerial()) {

        //a continuación ESCRIBE en el tag y luego lee lo que se escribió
        // Write data ***********************************************
        for (int i = 0; i < 4; i++) {
          //data is writen in blocks of 4 bytes (4 bytes per page)
          status = (MFRC522::StatusCode) mfrc522.MIFARE_Ultralight_Write(pageAddr + i, &buffer[i * 4], 4); //escribe el contenido del buffer en el tag
          if (status != MFRC522::STATUS_OK) {
            Serial.print(F("MIFARE_Read() failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
            return navegaRef;
          }
        }
        Serial.println(F("MIFARE_Ultralight_Write() OK "));
        Serial.println();
        memset(buffer, 0, sizeof(buffer));
        memset(dataChar, 0, sizeof(dataChar));


        // Read data ***************************************************
        Serial.println(F("Leyendo dato escrito ... "));
        //data in 4 block is readed at once.
        status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAddr, buffer, &size);

        if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Read() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
          return navegaRef;
        }

        digitalWrite(ledPin, HIGH);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Dato escrito: ");
        Serial.print(F("Dato escrito: "));
        //Dump a byte array to Serial

        for (byte i = 0; i < 16; i++) {
          Serial.write(buffer[i]);
          dataChar[i] = buffer[i];
          if (dataChar[i] != '\n') {//utilizo el \n para no rellenar el dato con el signo parecido a la E con ' ' se rellena
            lcd.setCursor(i, 1);
            lcd.print(dataChar[i]);//
          }
          buffer[i] = '\0';

        }



        Serial.println();


        delay(700);
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();

        digitalWrite(ledPin, LOW);

        for (byte i = 0; i < 16; i++) {
          dataChar[i] = '\0';
        }
        escritura = 1;

      }
      //return;
    }

    else {

      revisarWiFi();

    }

  }
  // enviarDatos();
  //return datoTag;
  */
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//******************************************************************
