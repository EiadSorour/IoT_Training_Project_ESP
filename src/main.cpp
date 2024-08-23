#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Keypad.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT_U.h>

// SSID and password for wifi network to be connected with
const char* ssid = "<SSID>";
const char* password = "<WIFI PASSWORD>";

// Important variables to publish/subscribe to HiveMQ broker
const char* broker = "<HiveMQ broker url>";                                     // Broker
const char* username = "<HiveMQ username>";                                     // username
const char* userPass = "<HiveMQ password>";                                     // password
long currentPublishTime , lastPublishTime;                                      // Time stamps for mqtt requests intervals
char tempMessage[30];                                                           // variable to hold temperature message
char humMessage[30];                                                            // variable to hold humidity message
char gasMessage[30];                                                            // variable to hold gas message
char doorState[30] = "closed";                                                  // variable to hold door state message
char someoneIn[30] = "no";                                                      // variable to hold someone in message
char alarmMessage[30] = "deactivate";                                           // variable to hold alarm message
char lightsMessage[30] = "off";                                                 // variable to hold lights message
char screenMessage[30] = "Welcome !";                                           // variable to hold screen message
const int port = 8883;                                                          // Port of HiveMQ
const int pubPeriod = 500;                                                      // Period between every publish
const String doorCorrectPassword = "<Your home door password>";                 // Home door password

// Certificate to connect successfully with HiveMQ broker
static const char* cert PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Used pins
int IR_PIN = GPIO_NUM_5;                                                // IR Sensor
int SERVO_PIN = GPIO_NUM_19;                                            // Servo pin
int GAS_PIN = GPIO_NUM_34;                                              // Gas sensor pin
int BUZZER_PIN = GPIO_NUM_18;                                           // Buzzer pin
int LED_RED = GPIO_NUM_4;                                               // RGB red leg pin
int LED_GREEN = GPIO_NUM_2;                                             // RGB green leg pin
int LED_BLUE = GPIO_NUM_15;                                             // RGB blue leg pin
byte rowPins[] = {GPIO_NUM_13,GPIO_NUM_12,GPIO_NUM_14,GPIO_NUM_27};     // keypad row pins (first 4 pins)
byte colPins[] = {GPIO_NUM_26,GPIO_NUM_25,GPIO_NUM_33,GPIO_NUM_32};     // keypad column pins (last 4 pins)

// Number of rows and columns in keypad
const byte ROWS = 4;                                            
const byte COLUMNS = 4;
// Define keymap for keypad as 2D matrix
const char keys[ROWS][COLUMNS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// Some objects
Servo servo;                                                                    // Servo object
WiFiClientSecure eiad;                                                          // WifiClientSecure object to be used in PubSubClient library
PubSubClient client(eiad);                                                      // PubSubClient Object
LiquidCrystal_I2C lcd(0x27,16,2);                                               // LCD object
Keypad keypad = Keypad(makeKeymap(keys) , rowPins, colPins, ROWS, COLUMNS);     // Keypad object
DHT dht(GPIO_NUM_23, DHT11);                                                    // DHT (temprature and humidity sensor)

// Variables to hold ir and door states
int PrevIRState = 1;
String doorPassword = "";

// Function to connect/reconnect client to HiveMQ broker if not connected 
void reconnect(){
    while(!client.connected()){
        // wait 5 seconds before every connection attempt
        delay(5000);
        Serial.println("Connecting to MQTT broker");
        if(client.connect("esp32", username, userPass)){
            Serial.println("connected to HiveMQ server");
            // Subscribe to needed topics after connected successfully
            client.subscribe("Flutter/screen");
            client.subscribe("Flutter/doorState");
            client.subscribe("Flutter/alarm");
            client.subscribe("Flutter/ledColor/red");
            client.subscribe("Flutter/ledColor/green");
            client.subscribe("Flutter/ledColor/blue");
            client.subscribe("Flutter/ledState");
        }
    }
}

// Callback function that will be excuted once we recieve a message from subscription topics
void callback(char* topic, byte* payload, unsigned int length){
    
    // Declare a variable to hold payload message
    String message;
    // Add every char of payload to 'str' variable 
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    // Topics routing
    // Excute appropriate code for every route and update messages (states) of the components to publish them later to HiveMQ broker
    if(strcmp(topic, "Flutter/ledState") == 0){
        if(message == "on"){
            analogWrite(LED_RED, 255);
            analogWrite(LED_GREEN, 255);
            analogWrite(LED_BLUE, 255);
            snprintf(lightsMessage, 50 , "on");
        }else{
            analogWrite(LED_RED, 0);
            analogWrite(LED_GREEN, 0);
            analogWrite(LED_BLUE, 0);
            snprintf(lightsMessage, 50 , "off");
        }
    }else if(strcmp(topic, "Flutter/alarm") == 0){
        if(message == "activate"){
            digitalWrite(BUZZER_PIN, HIGH);
            snprintf(alarmMessage, 50 , "activate");
        }else{
            digitalWrite(BUZZER_PIN, LOW);
            snprintf(alarmMessage, 50 , "deactivate");
        }
    }else if(strcmp(topic, "Flutter/doorState") == 0){
        if(message == "open"){
            servo.write(180);
            snprintf(doorState, 50 , "open");
        }else{
            servo.write(0);
            snprintf(doorState, 50 , "closed");
        }
    }else if(strcmp(topic, "Flutter/screen") == 0){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(message);
    }

    if(strcmp(topic, "Flutter/ledColor/red") == 0){
        int red = message.toInt();
        analogWrite(LED_RED, red);
        snprintf(lightsMessage, 50 , "on");
    }
    
    if(strcmp(topic, "Flutter/ledColor/green") == 0){
        int green = message.toInt();
        analogWrite(LED_GREEN, green);
        snprintf(lightsMessage, 50 , "on");
    }
    
    if(strcmp(topic, "Flutter/ledColor/blue") == 0){
        int blue = message.toInt();
        analogWrite(LED_BLUE, blue);
        snprintf(lightsMessage, 50 , "on");
    }
}

void setup() {

    // Begin serial monitor communication
    delay(2000);
    Serial.begin(115200);
    delay(2000);
    
    // Connect to wifi network
    WiFi.begin(ssid , password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nConnected to wifi");
    
    // Connect to HiveMQ broker
    eiad.setCACert(cert);               // Set certificate
    client.setServer(broker, port);     // Connect to the broker
    client.setCallback(callback);       // Set callback for any message from subscription topics

    // Set pin modes
    pinMode(IR_PIN,INPUT);
    pinMode(GAS_PIN,INPUT);
    pinMode(LED_RED,OUTPUT);
    pinMode(LED_GREEN,OUTPUT);
    pinMode(LED_BLUE,OUTPUT);
    pinMode(BUZZER_PIN,OUTPUT);

    // Set Lights and alaram to be "off" by default
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Attach servo object to its pin and close door by default
    servo.attach(SERVO_PIN);
    servo.write(0);

    // Begin LCD and display default message
    lcd.init();
    lcd.backlight();
    lcd.print(screenMessage);

    // Begin temperature and humidty sensor
    dht.begin();
}

void loop() {
    
    // If someone passed in front of ir sensor it will count him
    if(PrevIRState != digitalRead(IR_PIN) && PrevIRState == 1){
        PrevIRState = digitalRead(IR_PIN);
        snprintf(someoneIn, 50 , "yes");
    }

    // Get door password from keypad
    char keyPressed = keypad.getKey();
    if(keyPressed){
        // Letter 'A' test the enetered password so far
        if(keyPressed != 'A'){
            doorPassword += keyPressed;
        }else{
            // if password is correct open door and reset password else reset password only
            if(doorPassword == doorCorrectPassword){
                servo.write(180);
                doorPassword = "";
                snprintf(doorState, 50 , "open");
            }else{
                doorPassword = "";
            }
        }
    }

    // Check if client disconnected from HiveMQ broker
    if(!client.connected()){
        // Connect client again
        reconnect();
    }
    client.loop();

    // Publish IR sensor reading to HiveMQ broker every "period" of seconds
    currentPublishTime = millis();
    if(currentPublishTime - lastPublishTime > pubPeriod){
        // Update temperature,hmudity and gas messages to publish them
        snprintf(tempMessage, 50 , "%.2f" , dht.readTemperature());
        snprintf(humMessage, 50 , "%.2f" , dht.readHumidity());
        snprintf(gasMessage, 50 , "%lu" , map(analogRead(GAS_PIN) , 0 , 4095 , 0 ,100));
        // Publish to all topics
        client.publish("ESP/temperature" , tempMessage);
        client.publish("ESP/humidity" , humMessage);
        client.publish("ESP/gas" , gasMessage);
        client.publish("ESP/doorState" , doorState);
        client.publish("ESP/someoneIn", someoneIn);
        client.publish("ESP/alarm", alarmMessage);
        client.publish("ESP/ledState", lightsMessage);
        lastPublishTime = millis();
    }
}
