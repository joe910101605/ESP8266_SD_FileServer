#include "FileManager.h"

static int getFileCount(SdFile dir){
  int count = 0;
  SdFile file;
  while(file.openNext(&dir, O_RDONLY)){
    file.close();
    count++;
  }

  return max(count, 1);
}

void FileManager::setPath(String path, bool reload){
  if(path.equals(req_t.file_path) && !reload){return;}
  req_t.file_path = path;
  if(!dir.open(req_t.file_path.c_str())){
    Serial.println("dir.open failed");
    return;  
  }
  file_count = getFileCount(dir);
}

void FileManager::setPageNum(int num){
  req_t.page_num = num;
//  Serial.println(req_t.page_num);
}

void FileManager::setPageItem(int cnt){
  req_t.page_item = cnt;
//  Serial.println(req_t.page_item);
}

bool FileManager::initFile(){
  //初始化SPIFFS
  Serial.println("Initializing SPIFFS...");
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS initialization done!");
    //检查文件是否存在
    if (SPIFFS.exists("/index.html")) {
      Serial.println("index.html exists!"); 
    }
    else {
      Serial.println("index.html not found.");
      return false;
    }
  }else{
    Serial.println("SPIFFS initialization failed!");
    return false;
  }

  //初始化SD卡
  Serial.println("Initializing SD card...");
  if (!sd.begin(cs_pin, SPI_FULL_SPEED )) {
    Serial.println("initialization failed!");
    return false;
  }
  Serial.println("initialization done.");

  return true;
}

void FileManager::findFiles(AsyncJsonResponse * response){
  if(req_t.file_path == "")
    return;

  if(!dir.open(req_t.file_path.c_str())){
    Serial.println("dir.open failed");
    return;  
  }

  int page_count = file_count / req_t.page_item; //总页数
  int remain = (file_count % req_t.page_item);
  page_count = remain > 0 ? page_count+1 : page_count;
  req_t.page_num = min(req_t.page_num, page_count);
  
//  int page_item = (remain == 0 || req_t.page_num < page_count) ? min(req_t.page_item, PAGE_ITEM_DEFAULT) : remain;
  response->addHeader("Server","HCamera File Web Server");
  
  JsonObject& root = response->getRoot();
  root["pageNum"] = req_t.page_num;
  root["pageItem"] = req_t.page_item;
  root["pageCount"] = page_count;
  root["dir"] = req_t.file_path;
  JsonArray& file_array = root.createNestedArray("files");
//  Serial.println(file_array.success());

  int index = 0;
  int index_begin = (req_t.page_num-1) * req_t.page_item;
  int index_end = (req_t.page_num * req_t.page_item);
  while(file.openNext(&dir, O_RDONLY)){

    if (dir.getError() || index >= index_end) {
      break;
    }

    //按页数读取
    if(index >= index_begin){
      bool is_dir = file.isDir();
      JsonObject& nested = file_array.createNestedObject();

      char _c_tmp[128];
      file.getName(_c_tmp, sizeof(_c_tmp));
      
      nested["name"] = _c_tmp;
      nested["size"] = is_dir ? 0.0 : file.fileSize();

      uint16_t f_date, f_time;
      file.getCreateDateTime(&f_date, &f_time);

      memset(_c_tmp, 0, 128);
      sprintf(_c_tmp, \
              "%d-%02d-%02d %02d:%02d:%02d", \
              FS_YEAR(f_date), FS_MONTH(f_date), FS_DAY(f_date),\
              FS_HOUR(f_time), FS_MINUTE(f_time), FS_SECOND(f_time));

      nested["date"] = _c_tmp;
      nested["isDir"] = is_dir;
  
      file.close();
    }
    
    index++;
  }
  
}

/**
 * path: 路径
 * flag: oflag_t(int)
 */
FsFile FileManager::openFile(const char* path, int flag){
  return sd.open(path, flag); 
}

void FileManager::removeFile(const char* path){
  if(sd.exists(path)) sd.remove(path);
}

bool FileManager::existCheck(const char* path){
  return sd.exists(path);
}

void FileManager::makeDir(const char* path){
  sd.mkdir(path);
}

void FileManager::deleteFile(String path){
  FsFile file = sd.open(path.c_str(), O_RDONLY);
  if(!file.isDirectory()){
    file.close();
    sd.remove(path.c_str());
    Serial.println(path);
    return;
  }
  file.rewindDirectory();
  SdFile entry;
  while(entry.openNext(&file, O_RDONLY)){
    char _c_name[64];
    entry.getName(_c_name, sizeof(_c_name));
    String entryPath = path + "/" + _c_name;
    Serial.println(entryPath);
    if(entry.isDirectory()){
      entry.close();
      deleteFile(entryPath.c_str());
    } else {
      entry.close();
      sd.remove(entryPath.c_str());
    }
  }
  sd.rmdir(path.c_str());
  file.close();
}
