#include <AES.h>
#include <SPI.h>
#include <Bridge.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <BridgeClient.h>
#include <iBadgeShield.h>

iBadgeShield ibs(13, 5);
int count;
// Update these with values suitable for your network.
byte mac[]    = {  0xB4, 0x21, 0x8A, 0xF8, 0x10, 0xBC };
IPAddress ip(192, 168, 1, 12);
IPAddress server(198, 41, 30, 241);
BridgeClient ethClient;
PubSubClient client(ethClient);

// Define our server object
BridgeClient piClient;
bool Connected;

// Make the server connect to the desired server and port
IPAddress piaddr(192, 168, 1, 11);
#define PORT 5487

byte key[] = 
{
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
} ;
byte iv[16];
byte encbuffer[32] = {0};
AES aes;

void callback(char* topic, byte* payload, unsigned int length) {
  char encryptData[33];
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  if (length!=32) return;
  memset(encryptData, 0x00, 33);
  memcpy(encryptData, payload, length);
 
  int i;
  for(i = 0; i < 32; i++) encbuffer[i] = encryptData[i];
  String myData = do_decrypt();
  Serial.println(myData);
  
  char myArray[myData.length()+1];
  strcpy(myArray, myData.c_str());
  //發送解密資料給pi
  piClient.print(myArray);

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("giltest18")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("lalako","hello world");
      // ... and resubscribe
      client.subscribe("lalako");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 1 seconds before retrying
      delay(1000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Bridge.begin();
  Serial.println("iBadge Hello");
  count = 0;
  //iBadge
  ibs.init(13, 5);
  ibs.connectWifi("GILBERT1211", "gggggggg", "AES");
  ibs.authentication();
  //得到ibadge的Session key=================================
  String orikey = mySk();
  Serial.println("sk:[ " + orikey + " ]");
  for(int i = 0; i < 32; i++){
    key[i] = (char2int(orikey[i * 2]) << 4) + char2int(orikey[i * 2 + 1]);
  }
  aes.set_key(key, 256);
  for(int i = 0; i < 16; i++){
    iv[i] = 0;
  }
  //========================================================
  
  //mqtt
  client.setServer(server, 1883);
  client.setCallback(callback);
  Connected = false;
  Ethernet.begin(mac, ip);
  // Allow the hardware to sort itself out
  delay(500);
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (!Connected)
  {
    // Not currently connected, try to establish a connection
    piClient.connect(piaddr, PORT);
    if (piClient.connected())
    {
      Serial.println("Connected to the server.");   
      // Remember that there is a connection
      Connected = true;
    }
    else Serial.println("Could not connect to the server.");  
  }
  
  if (count++==0) {
    //告訴ibadge server我還活著
    ibs.heartbeat();  //send heartbeat
    //連接pi socket server
    if (Connected){
      if (piClient.connected());
      else{
         Serial.println("Server connection closed!");
         piClient.stop();
         Connected = false;
      }
    }
  } else {
    if (count>10) count = 0;
  }
  delay(1000);
}

int char2int(char c){
  if(c >= '0' && c <= '9'){
    return c - '0';
  }
  else if(c >= 'a' && c <= 'f'){
    return c - 'a' + 10;
  }
  else if(c >= 'A' && c <= 'F'){
    return c - 'A' + 10;
  }
  else{
    return 0;
  }
}

String do_decrypt(){
  byte encdata[16];  
  byte decresult[16];
  int i;
  String resultString = "";
  for(i = 0; i < 16; i++){
    encdata[i] = (char2int(encbuffer[i * 2]) << 4) + char2int(encbuffer[i * 2 + 1]);
  }
  //初始化iv
  for(int i = 0; i < 16; i++){
    iv[i] = 0;
    decresult[i] = 0;
  }
  //AES解密
  aes.cbc_decrypt(encdata, decresult, 1, iv);
  for(i = 0; i < 16; i++){
    if(decresult[i] == '\n' || decresult[i] == 0) break;
    resultString = resultString + (char)decresult[i];
  }

  return resultString;
}

String mySk(){
  
  String ret = "";
  int i;
  //送getsk cmd給ibadge
  String cmd = "ibadge dev_getsk\r"; 
  for (i=0; i < cmd.length(); i++) {
    char c = cmd.charAt(i);
    ibs.uartWrite(c);
  }
  
  while (!ibs.uartAvaiable()) delay(10); 
  
  while (ibs.uartAvaiable()) {
    char c = ' ';
    int decount = 65;
    while (decount--) {
      c = ibs.uartRead();      
      ret = ret + c;
      delay(5);
    }
    delay(5);
  }
  
  ret.trim();
  ret = ret.substring(18, 82);
  delay(50);  
  return ret;
}
