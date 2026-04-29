# Documentation about WOKWI

https://wokwi.com/

Add 2 ESP32-S3, modify the **diagram.json** file
```
{
  "version": 1,
  "author": "Anonymous maker",
  "editor": "wokwi",
  "parts": [
    {
      "type": "board-esp32-s3-devkitc-1",
      "id": "esp",
      "top": 0,
      "left": 0,
      "attrs": { "builder": "esp-idf" }
    },
    {
      "type": "board-esp32-s3-devkitc-1",
      "id": "esp2",
      "top": 9.42,
      "left": -139.43,
      "attrs": { "builder": "esp-idf" }
    }
  ],
  "connections": [ [ "esp:TX", "$serialMonitor:RX", "", [] ], [ "esp:RX", "$serialMonitor:TX", "", [] ],
  [ "esp2:TX", "$serialMonitor:RX", "", [] ], [ "esp2:RX", "$serialMonitor:TX", "", [] ] ],
  "dependencies": {}
}
```
<img width="512" height="571" alt="image" src="https://github.com/user-attachments/assets/beb88851-3f20-42c2-8e62-b27a43275d98" />
