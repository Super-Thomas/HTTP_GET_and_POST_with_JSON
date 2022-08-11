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

## HTTP
### Setting for IP


## Thanks to
[cJSON](https://github.com/DaveGamble/cJSON)
