/**
 * @网页文件管理
 * @作者：wiyixiao
 * @https://www.wiyixiao4.com/blog
 * @2022-03-18
 */
#include"FS.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESP8266mDNS.h>

#include "SdFat.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

#include "Constants.h"
#include "WifiAp.h"
#include "FileManager.h"

FileManager fileRoot(SPI_CS);
AsyncWebServer server(WIFI_PORT);
DNSServer dns;

const char* host = "FileServer";

void returnOK(AsyncWebServerRequest *request) {request->send(200, "text/plain", "");}
  
void returnFail(AsyncWebServerRequest *request, String msg) {request->send(500, "text/plain", msg + "\r\n");}

bool loadFromSdCard(AsyncWebServerRequest *request) {
  String path = request->url();
  String dataType = "text/plain";
  struct fileBlk {
    FsFile dataFile;
  };
  fileBlk *fileObj = new fileBlk;
  
  if(path.endsWith(URL_ROOT)){
    request->send(SPIFFS, "/index.html");
    return true;
  }
  else if(path.endsWith(URL_FAVICON));
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";

  fileObj->dataFile = fileRoot.openFile(path.c_str(), FILE_READ);

  if(fileObj->dataFile.isDirectory()){
    request->send(SPIFFS, "/index.html");
    return true;
  }

  if (!fileObj->dataFile){
    delete fileObj;
    return false;
  }

  if (request->hasParam("download")) dataType = "application/octet-stream";

  request->_tempObject = (void*)fileObj;
  request->send(dataType, fileObj->dataFile.size(), [request](uint8_t *buffer, size_t maxlen, size_t index) -> size_t {
                                              fileBlk *fileObj = (fileBlk*)request->_tempObject;
                                              size_t thisSize = fileObj->dataFile.read(buffer, maxlen);
                                              if((index + thisSize) >= fileObj->dataFile.size()){
                                                fileObj->dataFile.close();
                                                request->_tempObject = NULL;
                                                delete fileObj;
                                              }
                                              return thisSize;
                                            });

  return true;
}

void printDirectory(AsyncWebServerRequest *request){
  if( ! request->hasParam("dir")) return returnFail(request, "BAD ARGS");
  
  //Check if GET parameter exists
  AsyncWebParameter* p;
  bool reload = false;

  if(request->hasParam(URL_PARAMS_RELOAD)){
    p = request->getParam(URL_PARAMS_RELOAD);
    reload = p->value().equals("true");
  }

  if(request->hasParam(URL_PARAMS_DIR)){
    p = request->getParam(URL_PARAMS_DIR);
    fileRoot.setPath(p->value(), reload);
  }
  
  if(request->hasParam(URL_PARAMS_PN)){
    p = request->getParam(URL_PARAMS_PN);
    fileRoot.setPageNum(p->value().toInt());
  }

  if(request->hasParam(URL_PARAMS_PI)){
    p = request->getParam(URL_PARAMS_PI);
    fileRoot.setPageItem(p->value().toInt());
  }

  AsyncJsonResponse * response = new AsyncJsonResponse();
  fileRoot.findFiles(response);
  
  response->setLength();
  request->send(response);
}

void handleSDUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  struct uploadRequest {
    uploadRequest* next;
    AsyncWebServerRequest *request;
    FsFile uploadFile;
    uint32_t fileSize;
    uploadRequest(){next = NULL; request = NULL; fileSize = 0;}
  };
  static uploadRequest uploadRequestHead;
  uploadRequest* thisUploadRequest = NULL;

  if(!index){
    fileRoot.removeFile(filename.c_str());
    thisUploadRequest = new uploadRequest;
    thisUploadRequest->request = request;
    thisUploadRequest->next = uploadRequestHead.next;
    uploadRequestHead.next = thisUploadRequest;
    thisUploadRequest->uploadFile = fileRoot.openFile(filename.c_str(), FILE_WRITE);
    Serial.print("Upload: START, filename: "); Serial.println(filename);
  }
  else{
    thisUploadRequest = uploadRequestHead.next;
    while(thisUploadRequest->request != request) thisUploadRequest = thisUploadRequest->next;
  }
  
  if(thisUploadRequest->uploadFile){
    for(size_t i=0; i<len; i++){
      thisUploadRequest->uploadFile.write(data[i]);
    }
    thisUploadRequest->fileSize += len;
  }
  
  if(final){
    thisUploadRequest->uploadFile.close();
    Serial.print("Upload: END, Size: "); Serial.println(thisUploadRequest->fileSize);
    uploadRequest* linkUploadRequest = &uploadRequestHead;
    while(linkUploadRequest->next != thisUploadRequest) linkUploadRequest = linkUploadRequest->next;
    linkUploadRequest->next = thisUploadRequest->next;
    delete thisUploadRequest;
  }
}

void handleDelete(AsyncWebServerRequest *request){
  if( ! request->params()) returnFail(request, "No Path");
  String path = request->arg("path");
  if(path == "/" || !fileRoot.existCheck(path.c_str())) {
    returnFail(request, "Bad Path");
  }
  fileRoot.deleteFile(path);
  returnOK(request);
}

void handleCreate(AsyncWebServerRequest *request){
  if( ! request->params()) returnFail(request, "No Path");
  String path = request->arg("path");
  if(path == "/" || fileRoot.existCheck(path.c_str())) {
    returnFail(request, "Bad Path");
    return;
  }

  if(path.indexOf('.') > 0){
    FsFile file = fileRoot.openFile(path.c_str(), FILE_WRITE);
    if(file){
      file.write((const char *)0);
      file.close();
    }
  } else {
    fileRoot.makeDir(path.c_str());
  }
  returnOK(request);
}

void handleNotFound(AsyncWebServerRequest *request){
  String path = request->url();
  if(path.endsWith("/")) path += "index.htm";
  Serial.println("handler not found");
  if(loadFromSdCard(request)){
    return;
  }
  String message = "\nNo Handler\r\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET)?"GET":"POST";
  message += "\nParameters: ";
  message += request->params();
  message += "\n";
  for (uint8_t i=0; i<request->params(); i++){
    AsyncWebParameter* p = request->getParam(i);
    message += String(p->name().c_str()) + " : " + String(p->value().c_str()) + "\r\n";
  }
  request->send(404, "text/plain", message);
  Serial.print(message);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.autoConnect();

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println(".local");
  }

  //AP
//  while(!initWifi()){};
  Serial.println(getIP());

  //初始化文件系统
  while(!fileRoot.initFile());

  //初始化web服务
  initServer();

  Serial.println("HTTP server started");
}

void loop() {

}

void initServer(){
  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on("/edit", HTTP_POST, returnOK, handleSDUpload);
  server.onNotFound(handleNotFound);

  server.begin();
}
