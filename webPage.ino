#include <Ethernet.h>
#include <OneWire.h>
#include <SD.h>

//buffer size for processing html commands
#define MAXBUFF 40

//PINS
#define DS18B20 49 //DS18B20 pin used for the MAC address

//Generic HTML Commands
#define HTMLCONTENT F("Content-Type: text/html")
#define CLOSECONNECTION F("Connection: close")
#define DOCTYPEHTML F("<!DOCTYPE HTML>")

//EEPROM ADDRESSES
#define CRC32ADDRESS 0
#define USERADDRESS 4
#define NETWORKADDRESS 45

const byte port = 80;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};												//Set default MAC address to be replaced later with generated MAC address from DS18B20
EthernetServer server(port);

String readString;
byte i;
byte dsAddress[8];
OneWire ds(DS18B20);

void setup(){
	Serial.begin(38400);
	
	while(!SD.begin(4)){
		Serial.println("SD Failed");
	}

	if(networkConfig.dhcp){
		ds.reset_search();
		if(!ds.search(dsAddress)){
			Serial.println("Not found. Using default MAC address.");
		}else{
			Serial.println("success. Setting MAC address");
			mac[1] = dsAddress[3];	//Offset array to skip DS18B20 family code, and skip mac[0]
			mac[2] = dsAddress[4];
			mac[3] = dsAddress[5];
			mac[4] = dsAddress[6];
			mac[5] = dsAddress[7];
		}

		while(Ethernet.begin(mac) == 0){
			Serial.println("No Ethernet");
		}
		Serial.println(Ethernet.localIP());
	}

	server.begin();
}

void loop(){
// listen for incoming clients
	String s;
	EthernetClient client = server.available();
	if (client) {
		Serial.println("new client");
		// an http request ends with a blank line
		boolean currentLineIsBlank = true;
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				s+=c;

				if(c == '\n' && currentLineIsBlank){
					interpret(client, s);
				}
				if (c == '\n') {
					// you're starting a new line
					currentLineIsBlank = true;
				} else if (c != '\r') {
					// you've gotten a character on the current line
					currentLineIsBlank = false;
				}
			}
		}
	}
}

String getFileName(String &request){
	char req[MAXBUFF];
	(request.substring(request.indexOf('/'))).toCharArray(req, MAXBUFF);
	String fileName;

	for(int i = 0; i < strlen(req); i++){
		if(req[i] != '\n' && req[i] != '\r' && req[i] != ' ' && req[i] != '?'){
			fileName += req[i];
		}else{
			break;
		}
	}
	if(fileName.equals("/")){
		fileName = "index.htm";
	}
	return fileName;
}

void publish(EthernetClient &client, String fileName){
	String returnType;
	const String htmlcont = HTMLCONTENT;
	const String closeConnec = CLOSECONNECTION;


	if(fileName.endsWith(".htm") || fileName.endsWith(".html")){
		returnType = HTMLCONTENT;
	}else if(fileName.endsWith(".css")){
		returnType = F("Content-Type: text/css");
	}else if(fileName.endsWith(".js")){
		returnType = F("Content-Type: text/javascript");
	}else if(fileName.endsWith(".svg") || fileName.endsWith(".xml")){
		returnType = F("Content-Type: image/svg+xml");
	}else{
		returnType = HTMLCONTENT;
	}

	Serial.print("File Name: ");
	Serial.println(fileName);

	File current = SD.open(fileName);
	if(current){
		Serial.println("Serving");
		client.println(F("HTTP/1.1 200 OK"));
		client.println(returnType);
		client.println(closeConnec);  // the connection will be closed after completion of the response
		client.println();//10

		while(current.available()){
			client.write(current.read());
		}
		current.close();
		Serial.println("Served");
	}else{
		Serial.println("Failed to get file");
		client.println(F("HTTP/1.1 404 Not Found"));
		client.println(HTMLCONTENT);
		client.println(closeConnec);  // the connection will be closed after completion of the response
		client.println();
		client.println(DOCTYPEHTML);
		client.println(F("<html>"));
		client.println(F("<h1> Error 404 - File Not Found :( </h1>"));
		client.println(F("</html>"));
	}

	// give the web browser time to receive the data
	delay(1);
	client.stop();
	Serial.println(F("client disconnected"));
}

void interpret(EthernetClient &client, String &request){
	String message;
	String method = request.substring(0,request.indexOf(' '));

	Serial.print("Method: ");
	Serial.println(method);
	Serial.println(request);

	if(method.equals("GET") && request.indexOf('?') == -1){
	    publish(client, getFileName(request));
	}else if(method.equals("GET") && request.indexOf('?') > -1){
		String tmp = request.substring(request.indexOf('?'));
		message = tmp.substring(0, tmp.indexOf(' '));
		process(client, method, message);
	}else if(method.equals("POST")){
		while(client.available()){
			message += (char)client.read();
		}
		Serial.print("Post Message: ");
		Serial.println(message);
		process(client, method, message);
	}

	// give the web browser time to receive the data
	delay(1);
	client.stop();
	Serial.println(F("client disconnected"));
}

void process(EthernetClient &client, String &method, String &message){
	client.println(F("HTTP/1.1 200 OK"));
	client.println(HTMLCONTENT);
	client.println(CLOSECONNECTION);
	client.println();
	client.println(DOCTYPEHTML);
	client.println(F("<html>"));
	client.print("<h1> ");
	client.print(method);
	client.println(F(" TEST OK </h1>"));
	client.print("<p> ");
	client.print(message);
	client.println(" </p>");
	client.println(F("</html>"));
}