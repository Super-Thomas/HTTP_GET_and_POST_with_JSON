# HTTP_GET_and_POST_with_JSON

## Overview
Example of HTTP GET and POST with JSON on STM32F746 Discovery Board.

## Development environment
|Name|Description|Note|
|:------|:---|:---|
|STM32F746G-DISCO|Development board with ARM Cortex-M7 provided by STMicroelectronics|[Link](https://www.st.com/en/evaluation-tools/32f746gdiscovery.html)|
|STM32 Cube IDE|IDE|Version 1.5.1|
|LwIP_HTTP_Server_Netconn_RTOS|Example code from SDK provided by STMicroelectronics||

## cJSON API
Use the cJSON API for JSON.
### For Parsing JSON Data
JSON_Parse() function in main.c have parse JSON syntax using cJSON API.
### For Create JSON Data
JSON_Make() function in main.c have make JSON syntax using cJSON API.

## Settings
### Setting for IP
Set IP in Firmware for Connect to Internet. You can set it in "inc/main.h".<br />
If you use DHCP, enable define about USE_DHCP.
```c
#define USE_DHCP /* enable DHCP, if disabled static address is used */
```
If you use static IP, disable define about USE_DHCP and modify information of your IP/Netmask/Gateway.<br />
If in case IP address is 192.68.20.113, Netmask is 255.255.255.0, Gateway address is 192.68.20.1,
```c
//#define USE_DHCP /* enable DHCP, if disabled static address is used */

/*Static IP ADDRESS*/
#define IP_ADDR0   192
#define IP_ADDR1   68
#define IP_ADDR2   20
#define IP_ADDR3   113
   
/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/*Gateway Address*/
#define GW_ADDR0   192
#define GW_ADDR1   68
#define GW_ADDR2   20
#define GW_ADDR3   1
```

### Setting for lwIP
Change LWIP_SOCKET value 0 to 1 in "inc/lwipopts.h" because We will communicate HTTP protocol using TCP socket of lwIP.
```c
#define LWIP_SOCKET 1
```

## HTTP
### Send GET Reqeust to HTTP Server
You can send HTTP GET request to HTTP Server using HTTP_GET_Reqeust() function in "main.c".<br />
This example is connect to "openweathermap.org" and then receive weather information of Seoul after send HTTP GET request.
```http
GET /data/2.5/weather?q=Seoul,KR&units=metric&appid=9dacd623d8637f89267d33608c32700c HTTP/1.0\r\nHost:api.openweathermap.org\r\n\r\n\r\n
```
```c
// Configure HTTP header for GET request
uint8_t http_head[]="GET /data/2.5/weather?q=Seoul,KR&units=metric&appid=9dacd623d8637f89267d33608c32700c HTTP/1.0\r\nHost:api.openweathermap.org\r\n\r\n\r\n";
// Send HTTP header to HTTP server
ret = send(CControl, http_head, sizeof(http_head), 0);
```
Receive response data for HTTP GET request.
```c
// Check response
do {
	// Read response from HTTP server
	ret = read(CControl, &u8DataBuffer[nCounter], 1024);
	if (ret != 0) {
		if ((nCounter + ret) > 1023) {
			// Overrun
		}
		else {
			nCounter += ret;
		}
	}
} while(ret != 0);
```
JSON data received from HTTP Server is output to  LCD.
![image](https://user-images.githubusercontent.com/99227045/184095912-4f2adaa7-752c-4d7c-9bdf-1b265126255d.png)

### Send POST Reqeust to HTTP Server
You can send HTTP POST request to HTTP Server using HTTP_POST_Reqeust() function in "main.c".<br />
This example is connect to "webhook.site" and then check data using web browser after send HTTP POST request.
```http
POST http://webhook.site/9aa08aba-4ead-4fd9-9278-1ac46e0f3224 HTTP/1.0\r\nHost:webhook.site\r\nContent-Type:application/json\r\nContent-Length:[Size of body data]\r\n\r\n[Body data]
```
```c
// Configure HTTP header for POST request
uint8_t http_head[1024];
memset(&http_head[0], 0, sizeof(http_head));
sprintf(&http_head[0], "POST /9aa08aba-4ead-4fd9-9278-1ac46e0f3224 HTTP/1.0\r\nHost:webhook.site\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n%s",
	strlen(gu8DataBuffer),
	gu8DataBuffer);

// Send HTTP header to HTTP server
ret = send(CControl, http_head, sizeof(http_head), 0);
```
You can check data in link below.<br />
https://webhook.site/#!/9aa08aba-4ead-4fd9-9278-1ac46e0f3224/a1f651c7-f298-430a-8105-e0737f080b63/1
![image](https://user-images.githubusercontent.com/99227045/184097886-4de9b5aa-e929-46e4-a392-c190129452fe.png)

## Thanks to
[cJSON](https://github.com/DaveGamble/cJSON)
