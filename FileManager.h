#ifndef _FILE_MANAGER_H
#define _FILE_MANAGER_H

/**
 * SPI
 * MISO: 12
 * MOSI: 13
 * CLK:  14
 * CS:   15
 */

#include"FS.h"
#include "SdFat.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "Constants.h"

typedef struct RequestStruct{
  String file_path; //路径
  int page_num; //页数
  int page_item; //每页显示数量
}req_struct_t;

class FileManager{
private:
  int cs_pin;
  int file_count;
  SdFat sd;
  SdFile dir;
  SdFile file;
  req_struct_t req_t;
public:
  FileManager(int cs){
    cs_pin = cs;
    pinMode(cs_pin, OUTPUT);

    req_t.file_path = "";
    req_t.page_num = 1;
    req_t.page_item = 10;
  }
  ~FileManager(){}
  bool initFile();
  void setPath(String path, bool reload);
  void setPageNum(int num);
  void setPageItem(int cnt);
  void findFiles(AsyncJsonResponse * response);
  FsFile openFile(const char* path, int flag);
  void removeFile(const char* path);
  bool existCheck(const char* path);
  void makeDir(const char* path);
  void deleteFile(String path);
};

#endif
