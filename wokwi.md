# Documentation about WOKWI

https://wokwi.com/

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
