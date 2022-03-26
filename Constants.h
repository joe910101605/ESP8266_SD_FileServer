#ifndef _CONSTANTS_H
#define _CONSTANTS_H

//Wifi配置
#define WIFI_SSID   "ESP8266-AP"
#define WIFI_PASSWD "12345678"
#define WIFI_PORT   80

//URL配置
#define URL_ROOT "/"
#define URL_FAVICON "/favicon.ico"
#define URL_PARAMS_DIR "dir"
#define URL_PARAMS_PN "pn"
#define URL_PARAMS_PI "pi"
#define URL_PARAMS_RELOAD "reload"

//其他
#define SPI_CS 15
#define PAGE_ITEM_DEFAULT 6

static int strCompare(const char* s1, const char* s2){
    return strcmp(s1, s2);
}

#endif
