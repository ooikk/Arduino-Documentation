# ESP32-S3 N16R8 Memory Management, Heap, Stack and PSRAM

Senior Embedded Systems Architect • Arduino-ESP32 Core 3.x • ESP32-S3-WROOM-1-N16R8

The most important architectural distinction is that 16 MB Flash stores firmware and persistent data, 8 MB PSRAM provides additional runtime memory, and the internal SRAM remains limited to 512 KB nominally. These memories are not interchangeable: their accessibility, performance, allocation capabilities, and lifetime differ.

## 1. N16R8 memory map

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/81796aa5-c3de-4f87-a128-10eecbb43fa8" />

[ESP32 Flash/PSRAM & Memory Leaks: Understanding the Memory Map and Preventing Crashes | SunFounder](https://images.openai.com/static-rsc-4/8zeBjDYIpC27eOTBZbr9cEZ2YqbFUkc9X3WhWhK0AzpshnmLlHJgE6vLWx2sO3WTizuDRMzw71LlT4NBFglO0jNNBQ2ZXI_avH8fhtq2FrgSheUH8FrPfNaBj0x_Ky0MnWhpShjWfC9wSbN85bwmb5MCgy7KEbYvjxotf3NQQGg?purpose=inline)

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/d80aa500-7ac9-4712-bf27-7af8beae4d86" />


[ESP32uesday: Pondering LCDs and Using Espressif DevKits w/Arduino IDE «  Adafruit Industries – Makers, hackers, artists, designers and engineers!](https://images.openai.com/static-rsc-4/wHxGKETs3owfY9JovpccUc5wmTn75IPNRW-xs5Yh5xs0-ahLX62mu6unPx1OrVpX-nG_MHS_3ThDtcdvIsFtFdT06cjCTJsiIHgFAxb-0d5C5J8vlXXIiBLvIevtAYAxP00ZqzT1UMc0or2PikrQ0qu-gRVL305GKUNF7Ob585U?purpose=inline)

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/c453ada9-251b-441a-9f7c-ece03316e2a6" />


[Datasheets](https://images.openai.com/static-rsc-4/vyhAEoU0K8-GKxc1WgBZPBWOz9XX8AA-w9VTMRZyGj1P4jaeovR6auJaBPyaejQkhY7orHBo2Yj8GW5n46JAZouNbBUGwE570sSBrBBnGhG3uD0u-1eamTgoVS8qfmzuOgS_hFAqQLtHzO7Z5aRsrd2KbYUdgnZLyxKH1r1hk7E?purpose=inline)

### 1.1 Physical memory architecture

```
             ESP32-S3-WROOM-1-N16R8
┌─────────────────────────────────────────────┐
│             ESP32-S3 SoC                    │
│                                             │
│  Internal SRAM: 512 KB nominal              │
│  ┌───────────────────────────────────────┐  │
│  │ IRAM: instruction-capable SRAM        │  │
│  │ DRAM: data-capable SRAM               │  │
│  │ Shared/reserved areas and system use  │  │
│  └───────────────────────────────────────┘  │
│                  │                          │
│  ┌───────────────┴───────────────────────┐  │
│  │ MSPI / cache subsystem                │  │
│  └───────────────┬───────────────────────┘  │
└──────────────────┼──────────────────────────┘
                   │ Octal SPI / MSPI interface
          ┌────────┴─────────┐
          │                  │
┌─────────▼────────┐ ┌───────▼─────────────┐
│ 16 MB Flash      │ │ 8 MB PSRAM          │
│ Non-volatile     │ │ Volatile runtime RAM│
│ XIP code/data    │ │ Cached external RAM │
└──────────────────┘ └─────────────────────┘
```

Conceptual architecture, not a pin-level schematic or an exact silicon address map.

The ESP32-S3 uses a cache-backed external-memory architecture. Flash and PSRAM are external memory devices inside the module, even though both are physically embedded in the WROOM-1 package.

| Memory        | Capacity       | Primary purpose                                             | Retains data without power? |
| ------------- | -------------- | ----------------------------------------------------------- | --------------------------- |
| Internal SRAM | 512 KB nominal | Runtime data, stacks, heap, instructions and system buffers | No                          |
| Flash         | 16 MB          | Firmware, constants, filesystems and persistent storage     | Yes                         |
| OPI PSRAM     | 8 MB           | Large runtime buffers, arrays, images and application data  | No                          |

Important: The 512 KB SRAM is not all available to your application. Some is reserved or used by the ROM, runtime, caches, system components and other architectural requirements. The free heap can be much smaller, especially after Wi-Fi, networking and RTOS services start.

### 1.2 Flash versus PSRAM: XIP and cached external RAM

Flash — execute in place (XIP)

- Stores the firmware image and read-only program data.
- Code and constants can be mapped into the processor's address space through the cache.
- Normal program execution does not require copying the entire firmware into SRAM.
- Flash is non-volatile and is not a substitute for writable runtime heap memory.
- Flash erase/write operations have special restrictions because code and data access can be affected while the flash subsystem is busy.

PSRAM — external volatile RAM

- Extends the available runtime data capacity by approximately 8 MB.
- Is accessed through the memory/cache subsystem over the Octal SPI interface.
- Supports large writable allocations, but is generally slower and less predictable than internal SRAM.
- Loses its contents on power removal or reset.
- Is not automatically suitable for every use of internal RAM, particularly DMA buffers and latency-sensitive code/data.

### 1.3 Why OPI matters on N16R8

OPI means Octal Peripheral Interface in this memory context: eight data lines are used for the memory transfer protocol. QSPI uses four data lines.

The N16R8 configuration uses Octal Flash and Octal PSRAM with the ESP32-S3 memory interface. This gives the memory subsystem more data-transfer capacity than a comparable four-bit interface under similar conditions.

However, three qualifications matter:

1. Flash and PSRAM do not necessarily operate at identical clock rates or achieve identical effective bandwidth.
2. Cache misses, access patterns, transaction overhead and memory-controller scheduling affect actual performance.
3. Sharing the MSPI resources means external-memory activity can contend for bandwidth. OPI does not make PSRAM as fast as internal SRAM.

Do not interpret the Arduino IDE's `Flash Mode = QIO` label as proof that your N16R8 module physically uses a four-bit Flash device. The exact configuration depends on the board package, bootloader, memory initialization and selected board settings. Verify that the installed ESP32-S3 board definition correctly configures the module's Octal Flash and PSRAM.

### 1.4 Where program sections reside

| Section    | Typical placement                                                                      | Meaning                                             |
| ---------- | -------------------------------------------------------------------------------------- | --------------------------------------------------- |
| `.text`    | Flash-mapped IROM; selected code may reside in IRAM                                    | Executable instructions                             |
| `.rodata`  | Flash-mapped DROM; selected data may be copied elsewhere                               | Read-only constants                                 |
| `.data`    | Internal DRAM at runtime                                                               | Initialized writable globals/statics                |
| `.bss`     | Internal DRAM at runtime                                                               | Zero-initialized globals/statics                    |
| Heap       | Internal RAM, PSRAM, or both depending on allocator/capabilities                       | Dynamic allocations                                 |
| Task stack | Normally internal RAM by default; placement depends on task creation API/configuration | Local variables, call frames and saved task context |
| RTC memory | RTC fast/slow memory where supported                                                   | Special retention and low-power use cases           |

The compiler and linker determine section placement. A declaration such as `const` does not universally guarantee a particular physical placement: inspect the linker map and the actual ESP-IDF/Arduino build configuration when placement matters.

## 2. Partition schemes for 16 MB Flash

The Flash chip's capacity and the maximum firmware size are two different things.

- Flash capacity: 16,777,216 bytes.
- Application capacity: determined by the selected partition table and OTA layout.
- Filesystem capacity: the Flash space assigned to SPIFFS, LittleFS or FATFS.
- Runtime PSRAM: separate from Flash partitions; it does not increase the maximum firmware image size.

### 2.1 How Flash is partitioned

```
16 MB Flash — conceptual layout

0x000000 ┌──────────────────────────┐
         │ Bootloader / reserved    │
         ├──────────────────────────┤
         │ Partition table          │
         ├──────────────────────────┤
         │ NVS                      │
         ├──────────────────────────┤
         │ OTADATA (if OTA enabled) │
         ├──────────────────────────┤
         │ ota_0 / app0             │
         │ Firmware image           │
         ├──────────────────────────┤
         │ ota_1 / app1 (optional)  │
         │ Second firmware image    │
         ├──────────────────────────┤
         │ SPIFFS / LittleFS / FATFS│
         │ Files, images, logs      │
         └──────────────────────────┘
16 MB    End of Flash
```

The drawing is conceptual; the actual offsets, alignment, reserved areas and partition sizes are defined by the partition table.

| Partition         | Purpose                                                                        |
| ----------------- | ------------------------------------------------------------------------------ |
| `nvs`             | Persistent key-value settings, Wi-Fi credentials and application configuration |
| `otadata`         | Selects the active OTA application slot and tracks OTA state                   |
| `ota_0` / `ota_1` | Firmware slots used for OTA updates                                            |
| `factory`         | Optional factory firmware image                                                |
| `spiffs`          | SPIFFS filesystem partition                                                    |
| `littlefs`        | LittleFS partition, if supported by the selected library and partition setup   |
| `fatfs`           | FAT filesystem partition, commonly used for files and assets                   |

NVS, OTA metadata and the partition table occupy Flash too. Not all 16 MB can be allocated to a firmware image.

#### SPIFFS, LittleFS, and FATFS on ESP32-S3

SPIFFS, LittleFS, and FATFS are different file system choices used to manage, store, and organize files in the flash memory (or external storage) of an ESP32-S3 microcontroller. Because raw flash memory cannot handle files and directories natively, these libraries act like the operating system's file manager (similar to NTFS on Windows or APFS on macOS) but are customized for tiny, low-power hardware.

##### Comparison

| Feature | SPIFFS | LittleFS | FATFS |
|---|---|---|---|
| Status | ⚠️ Deprecated (Legacy) | 🏆 Recommended (Standard) | 💾 Best for SD Cards |
| Directory Support | ❌ Flat structure (no true folders) | ✅ True nested directories | ✅ True nested directories |
| Power-Loss Safety | ⚠️ Partial (files can corrupt) | ✅ Excellent (crash-proof/atomic) | ❌ Poor (high risk of corruption) |
| Wear Leveling | ✅ Integrated (static) | ✅ Integrated (dynamic & robust) | ⚠️ Needs extra driver layer |
| RAM Consumption | 🟢 Low | 🟢 Bounded / very low (~4 KB) | 🟡 Medium (~12 KB) |
| Read Speed | 🟡 Medium | 🟡 Medium (slower appends) | ⚡ Fast (excellent for large files) |
| Primary Media | Internal SPI Flash only | Internal / external flash | SD cards / USB / flash |

##### 1. SPIFFS (SPI Flash File System)

SPIFFS is the original, legacy file system used in early ESP32 development.

- **How it works:** It pretends directories exist by using the forward slash in filenames (e.g., saving a file named `/images/pic.png` in a flat list).
- **The catch:** It is no longer actively maintained by the community. As the memory fills past 70%, its performance drops drastically, and it struggles with file fragmentation. 

##### 2. LittleFS (Little File System)

Created by ARM specifically for embedded microcontrollers, LittleFS is now the default go-to standard for the ESP32-S3's internal onboard flash.

- **Power resilience:** It uses "atomic" writes. If the ESP32-S3 suddenly loses power or reboots right in the middle of writing a file, LittleFS reverts back to the last good state instead of bricking the whole file system. 
- **Smart wear leveling:** Flash memory blocks degrade if written to repeatedly. LittleFS automatically cycles data across physical memory blocks to maximize the lifespan of your ESP32-S3 hardware. 

##### 3. FATFS (File Allocation Table File System)

This is a micro-version of the universal FAT system used by Windows and DOS. 

- **The compatibility king:** Because it follows standard computer formatting rules, any files written by the ESP32-S3 to a FATFS partition can be read natively by plugging the media into a computer. 
- **The catch:** It is not power-safe. A sudden power loss can break the whole file structure. It also lacks internal wear leveling, meaning using it directly on the ESP32-S3's raw internal flash memory can burn out the chip quickly unless wrapped in Espressif's custom wear-leveling driver layer. 

##### Which One Should You Choose?

- **Use LittleFS** if you are storing internal settings, web server files (HTML/CSS), or small local data logs directly inside the ESP32-S3's flash chip. It is safer, cleaner, and the modern standard. 
- **Use FATFS** only if your ESP32-S3 project is wired to an external MicroSD Card slot or an external USB flash drive. Do not use it for internal flash unless you require high-speed reading of massive, static files (like large audio/video playbacks). 
- **Avoid SPIFFS** for any new firmware developments. 



### 2.2 Compare the partition strategies

| Strategy                                  | Application space             | Filesystem space                    | OTA capability                     | Best use                                        |
| ----------------------------------------- | ----------------------------- | ----------------------------------- | ---------------------------------- | ----------------------------------------------- |
| 16 MB, single large app + large FATFS     | One large app slot            | Large                               | No two-slot rollback/update layout | Large firmware or file storage                  |
| 16 MB, two approximately 6.5 MB app slots | Approximately 6.5 MB per slot | Small or absent, depending on table | Yes                                | Reliable OTA with larger firmware               |
| Custom 16 MB CSV                          | You choose the slot sizes     | You choose the filesystem size      | Depends on table                   | N16R8 projects needing both OTA and large files |

About the names: Arduino-ESP32 menu labels and exact built-in partition layouts can vary with the installed board package. A displayed scheme such as `16M (3MB APP/9.9MB FATFS)` or `16M (2x 6.5MB OTA)` should be checked against the actual CSV supplied by your installed Core 3.x package. The labels alone are not a substitute for the partition table.

A 6.5 MB OTA slot does not mean you can upload a 16 MB sketch. The firmware image must fit into the selected app slot, including any relevant image-format overhead.

### 2.3 Example custom `partitions.csv`

This illustrative table reserves two 6 MB OTA slots and approximately 3.5 MB for a filesystem, with the remaining space for system partitions and alignment. Always verify that the offsets and sizes fit the 16 MB address range and satisfy the partition-table constraints.

```
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x6000,
otadata,  data, ota,     0xF000,   0x2000,
app0,     app,  ota_0,   0x20000,  0x600000,
app1,     app,  ota_1,   0x620000, 0x600000,
spiffs,   data, spiffs,  0xC20000, 0x380000,
```

The app slots are 6 MiB each, and the filesystem is 3.5 MiB. This example is intentionally conservative about leaving space unused; it does not allocate every byte of the 16 MB device. You can expand the filesystem only after accounting for the end offset, bootloader/table placement and any required reserved space. The filesystem subtype `spiffs` may also be used with a compatible filesystem implementation; it does not automatically turn the partition into LittleFS.

### 2.4 Install a custom partition table

1. Locate the Arduino-ESP32 package's partition CSV files in your Arduino data directory.
2. Copy the closest compatible 16 MB partition CSV into your sketch folder.
3. Rename the copy to `partitions.csv`.
4. Edit the sizes and offsets, checking the total address range and alignment.
5. Select a supported custom/default-with-custom-CSV partition option if your installed board package exposes one. Some Arduino-ESP32 versions require a custom board definition or build configuration to select a sketch-local CSV.
6. Compile and inspect the build output for the partition table and app image sizes.
7. If changing the partition layout, perform a full erase/reflash when necessary, because existing NVS and filesystem contents may no longer match the new layout.

For ESP-IDF projects, set the custom partition table explicitly:

```
# sdkconfig.defaults
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

These are ESP-IDF configuration settings; they are not directly entered as ordinary Arduino sketch code.

Trainer's rule: choose the partition layout before deciding how large your firmware and filesystem can grow. A 16 MB Flash module does not guarantee a 16 MB application slot.

## 3. Runtime memory: stack, heap and PSRAM

### 3.1 Stack versus heap

The ESP32-S3 uses two Xtensa LX7 cores. Each FreeRTOS task has its own stack, and the two cores share the system's available memory pools.

```
Internal DRAM / RAM heap
┌──────────────────────────────────┐
│ Global and static writable data  │
├──────────────────────────────────┤
│ Free heap blocks                 │
│ malloc / new / internal buffers  │
├──────────────────────────────────┤
│ Task stack: Task A               │
│ Local variables / call frames    │
├──────────────────────────────────┤
│ Task stack: Task B               │
│ Local variables / call frames    │
└──────────────────────────────────┘

PSRAM heap
┌──────────────────────────────────┐
│ Large buffers                    │
│ Image / sensor / file data       │
│ Large arrays and application data│
└──────────────────────────────────┘
```

The diagram shows conceptual allocations, not a guaranteed physical ordering.

| Property          | Stack                                                    | Heap                                                     |
| ----------------- | -------------------------------------------------------- | -------------------------------------------------------- |
| Allocation        | Automatically used by function calls; reserved per task  | Allocated explicitly or by libraries                     |
| Lifetime          | Local variables normally last until the function returns | Until explicitly freed or the owning object is destroyed |
| Growth            | On Xtensa, a stack normally grows toward lower addresses | Allocator-dependent; free blocks may be split or merged  |
| Typical errors    | Stack overflow, deep call chains, large local arrays     | Allocation failure, leaks, fragmentation, double free    |
| PSRAM suitability | Usually internal RAM by default                          | Excellent for large PSRAM-capable allocations            |

Do not assume that every task stack grows in the same physical direction on every architecture. On ESP32-S3 Xtensa, the normal stack grows down toward lower addresses. The heap allocator manages separate regions and does not simply grow upward until it collides with the stack.

#### Heap Memory

In memory architecture, the **heap** is a region of a computer's RAM used for **dynamic memory allocation at runtime**. Unlike the stack, which follows a strict Last-In, First-Out (LIFO) order for function calls and local variables, the heap provides a large, unstructured pool of memory where data can be allocated and freed in any arbitrary order. 

##### Key Characteristics of Heap Memory

- **Dynamic Lifecycle:** The size of the data does not need to be known at compile time. Memory is requested on demand while the program is running.
- **Global Lifetime:** Data stored on the heap persists beyond the scope of the function that created it. It remains alive until it is explicitly deleted or cleaned up by the system.
- **Shared Across Threads:** In multi-threaded applications, the heap is shared among all threads, allowing them to share objects and data. This requires careful synchronization to avoid data races.
- **Pointer-Based Access:** When memory is allocated on the heap, the operating system or runtime returns a pointer (a memory address). This pointer itself is typically stored on the stack.

##### How the Heap Is Managed

Because the heap does not automatically clean itself up when a function ends, it must be managed in one of two ways:

- **Manual Management (e.g., C, C++):** Programmers explicitly request memory using functions like `malloc()` or operators like `new`, and must manually free it using `free()` or `delete`.
- **Automatic Management (e.g., Java, Python, Go):** The language's runtime environment uses an automatic [Garbage Collector (GC)](https://muratakkan.medium.com/java-memory-management-101-heap-memory-ca4060b10685) to periodically scan the heap, find objects that are no longer referenced, and safely reclaim their memory.

##### Stack vs. Heap Comparison

| Feature | Stack Memory | Heap Memory |
|---|---|---|
| Allocation Type | Static / automatic (compile-time size) | Dynamic (runtime-determined size) |
| Access Speed | Very fast (direct pointer manipulation) | Slower (requires pointer dereferencing) |
| Data Structure | Linear / LIFO | Unstructured / random |
| Size Limit | Small and fixed (OS dependent, e.g., 1–8 MB) | Large (limited only by system virtual memory) |
| Common Risks | Stack overflow (running out of stack space) | Memory leaks and fragmentation |

##### Common Heap Pitfalls

- **Memory Leaks:** Occur when memory is allocated on the heap but the program loses the pointer to it without freeing it. This permanently consumes memory until the application closes.
- **Fragmentation:** Over time, as memory blocks of various sizes are allocated and deleted randomly, the heap becomes fragmented. This leaves small gaps of unused memory, which can prevent the system from allocating a single large contiguous block later on.

> **Note:** The memory-architecture **heap** should not be confused with the **heap data structure**, which is a specialized tree-based data structure used to implement priority queues.



### 3.2 Arduino-ESP32 Core 3.x memory APIs

| API                                      | Meaning                                                                            |
| ---------------------------------------- | ---------------------------------------------------------------------------------- |
| `ESP.getFreeHeap()`                      | Free heap bytes in the default/general heap reporting path                         |
| `ESP.getMinFreeHeap()`                   | Minimum free heap observed since startup, useful for detecting worst-case pressure |
| `ESP.getMaxAllocHeap()`                  | Largest allocatable block in the relevant default heap                             |
| `ESP.getHeapSize()`                      | Total heap bytes exposed through the corresponding general heap reporting API      |
| `ESP.getPsramSize()`                     | Detected/configured PSRAM capacity                                                 |
| `ESP.getFreePsram()`                     | Free PSRAM bytes                                                                   |
| `ESP.getMaxAllocPsram()`                 | Largest allocatable PSRAM block                                                    |
| `psramFound()`                           | Whether PSRAM initialization/detection succeeded                                   |
| `heap_caps_get_free_size(caps)`          | Free bytes in heap regions matching requested capabilities                         |
| `heap_caps_get_largest_free_block(caps)` | Largest contiguous block matching the requested capabilities                       |

Use `heap_caps_get_free_size()` and `heap_caps_get_largest_free_block()` when you need precise capability-specific measurements rather than a single aggregate figure.

### 3.3 Internal heap versus PSRAM heap

The following is a typical illustrative startup snapshot, not a guaranteed reading for every N16R8 board.

| Measurement                 | Illustrative result                                   |
| --------------------------- | ----------------------------------------------------- |
| Detected PSRAM              | 8,388,608 bytes                                       |
| Free PSRAM                  | Around 8,000 KB before large allocations              |
| Free internal heap          | Around 300 KB, depending on startup and libraries     |
| Largest internal allocation | Often smaller than total free internal heap           |
| Largest PSRAM allocation    | Depends on fragmentation and reserved/runtime regions |

Why can free heap be approximately 300 KB even though the module contains 512 KB internal SRAM? Because the 512 KB is a nominal silicon capacity, not a promise of 512 KB available for application allocations. Some memory is used or reserved for instruction/data mapping, runtime operation and system services.

### 3.4 The crucial difference: free bytes versus largest block

Suppose your internal heap reports:

```
Free internal heap:          280,000 bytes
Largest internal free block: 190,000 bytes
```

A single 220 KB allocation can fail even though the sum of free bytes exceeds 220 KB. The allocator needs a sufficiently large contiguous region.

The same principle applies to PSRAM:

```
Free PSRAM:                   7,900,000 bytes
Largest PSRAM free block:     7,600,000 bytes
```

A 7.8 MB request may fail because no single suitable block is large enough. Fragmentation, allocator metadata and alignment all affect the result.

Use this rule: monitor both total free memory and the largest free block, particularly before allocating large images, camera frames, audio buffers or file caches.

### 3.5 Capability-aware allocation

ESP-IDF provides a capability allocator so that an application can request memory with specific properties.

```
#include "esp_heap_caps.h"

// General writable PSRAM allocation
uint8_t *image = (uint8_t *)heap_caps_malloc(
    1024 * 1024,
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
);

if (image == nullptr) {
    Serial.println("PSRAM allocation failed");
} else {
    // Use image...
    free(image);
}

// Internal RAM, suitable for DMA
uint8_t *dmaBuffer = (uint8_t *)heap_caps_malloc(
    4096,
    MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA
);

if (dmaBuffer == nullptr) {
    Serial.println("Internal DMA buffer allocation failed");
} else {
    // Use dmaBuffer with the relevant peripheral...
    free(dmaBuffer);
}
```

| Capability            | Practical interpretation                                               |
| --------------------- | ---------------------------------------------------------------------- |
| `MALLOC_CAP_INTERNAL` | Memory in internal RAM                                                 |
| `MALLOC_CAP_SPIRAM`   | External PSRAM                                                         |
| `MALLOC_CAP_8BIT`     | Supports byte-addressable data access                                  |
| `MALLOC_CAP_DMA`      | Memory suitable for the allocator's supported DMA requirements         |
| `MALLOC_CAP_32BIT`    | Only 32-bit accesses are guaranteed; not a general-purpose byte buffer |

Do not combine capabilities without understanding their intersection. For example, `MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA` deliberately asks for internal DMA-capable RAM.

### 3.6 DMA, ISR and Wi-Fi buffers: an important qualification

For the most portable Arduino-ESP32 Core 3.x design, allocate critical DMA buffers in internal DMA-capable RAM:

```
uint8_t *buffer = (uint8_t *)heap_caps_malloc(
    8192,
    MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA
);
```

There is an important ESP32-S3 nuance: newer ESP-IDF versions have hardware support for some DMA transfers involving PSRAM, subject to alignment, peripheral, descriptor-placement and driver restrictions. Therefore, the absolute statement that all PSRAM DMA is impossible is too broad. The safe default for a beginner's SPI, I2S or peripheral DMA buffer is internal DMA-capable memory unless the specific driver and configuration explicitly support external RAM.

Similarly, don't assume that every Wi-Fi or ISR-related buffer can be allocated in PSRAM. Some buffers and data accessed while the flash cache is disabled must reside in suitable internal memory. Keep latency-critical ISR data and code in the appropriate internal memory regions.

For authoritative restrictions and capability details, see the Espressif documentation: [ESP32-S3 external RAM guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/external-ram.html?utm_source=chatgpt.com) and [ESP32-S3 heap memory allocation guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/mem_alloc.html?utm_source=chatgpt.com).


## 4. Declaration and allocation reference table

### 4.1 Where should each variable go?

| Declaration / API                                               | Typical placement                                                 | Use it for                                                                                    |
| --------------------------------------------------------------- | ----------------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| `static const uint8_t logo[] PROGMEM`                           | Flash-mapped read-only data, subject to compiler/linker placement | Immutable logo, lookup table                                                                  |
| `const uint8_t logo[]`                                          | Often Flash-mapped read-only data when eligible                   | Constants                                                                                     |
| `DRAM_ATTR`                                                     | Internal DRAM                                                     | Writable or specially placed data requiring internal data RAM                                 |
| `IRAM_ATTR`                                                     | Internal instruction RAM                                          | Code that must execute while Flash cache is disabled, subject to full call-chain restrictions |
| `RTC_DATA_ATTR`                                                 | RTC memory with initialization/retention semantics                | Data retained across supported deep-sleep cycles                                              |
| `RTC_NOINIT_ATTR`                                               | RTC memory without normal startup initialization                  | Data that must survive startup initialization under supported conditions                      |
| `WORD_ALIGNED_ATTR`                                             | Alignment attribute                                               | Data requiring word alignment for a particular driver or access requirement                   |
| `malloc()` / `calloc()`                                         | Default heap policy; may include PSRAM depending on configuration | General dynamic allocations                                                                   |
| `heap_caps_malloc(..., MALLOC_CAP_INTERNAL)`                    | Internal RAM                                                      | Data requiring internal memory                                                                |
| `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`                      | PSRAM                                                             | Large external-memory buffers                                                                 |
| `heap_caps_malloc(..., MALLOC_CAP_SPIRAM \\| MALLOC_CAP_8BIT)`  | Byte-addressable PSRAM                                            | Byte arrays, images, text buffers                                                             |
| `heap_caps_malloc(..., MALLOC_CAP_INTERNAL \\| MALLOC_CAP_DMA)` | Internal DMA-capable RAM                                          | Conservative DMA buffers                                                                      |
| `ps_malloc()` / `ps_calloc()`                                   | Arduino PSRAM allocation wrappers                                 | Large arrays and buffers                                                                      |
| `new` / `delete`                                                | C++ default allocation policy                                     | Objects; placement depends on heap configuration                                              |
| `uint8_t buf[1024]` inside a function                           | Current task's stack                                              | Small, short-lived local buffer                                                               |

### 4.2 `PROGMEM` on ESP32-S3

For normal ESP32-S3 Arduino builds, `PROGMEM` is primarily a compatibility convention. Flash-mapped constants are accessed through the memory map; the AVR model of requiring special Flash-read instructions does not generally apply.

```
#include <Arduino.h>

static const uint8_t logo[] PROGMEM = {
    0x12, 0x34, 0x56, 0x78
};

void setup() {
    Serial.begin(115200);
    delay(500);

    for (size_t i = 0; i < sizeof(logo); ++i) {
        Serial.printf("%02X ", logo[i]);
    }
    Serial.println();
}

void loop() {}
```

Expected output:

```
12 34 56 78
```

The example illustrates normal access, not a guarantee of a particular linker section. Check the ELF/map file if the exact placement is critical.

### 4.3 `const`, `static`, `static const` and `constexpr`

| Keyword        | Meaning                                                                                                                |
| -------------- | ---------------------------------------------------------------------------------------------------------------------- |
| `const`        | Prevents modification through that object/reference; does not by itself guarantee Flash placement                      |
| `static`       | Gives a local variable static lifetime, or gives a file-scope object internal linkage                                  |
| `static const` | Combines static storage/linkage behavior with read-only access                                                         |
| `constexpr`    | Requires a compile-time constant expression where applicable; does not universally dictate the physical memory section |

Examples:

```
const int threshold = 25;

static int sampleCount = 0;

static const uint16_t gammaTable[] = {
    0, 100, 250, 500, 1000
};

constexpr size_t FRAME_BYTES = 320 * 240 * 2;
```

`const` does not mean “automatically placed in PSRAM,” and `constexpr` does not mean “automatically placed in internal RAM.” Physical placement depends on the toolchain, section attributes and linker configuration.

### 4.4 Stack arrays versus heap arrays

```
void processSample() {
    uint8_t smallBuffer[1024];  // Stack allocation

    // The array ceases to exist when this function returns.
}
```

The stack is convenient, but allocating large arrays locally can overflow a task's stack. A 100 KB local array is a poor default design.

For a large buffer, allocate from PSRAM:

```
uint8_t *buffer = (uint8_t *)ps_malloc(100 * 1024);

if (buffer == nullptr) {
    Serial.println("Allocation failed");
    return;
}

// Use buffer here.

free(buffer);
```

Always match allocation and release. The `ps_malloc()` wrapper returns memory that can normally be released with `free()` in Arduino-ESP32, because it uses the ESP-IDF heap allocator.

### 4.5 `malloc`, `calloc`, `realloc`, `free`

```
size_t bytes = 100 * 1024;

uint8_t *p = (uint8_t *)malloc(bytes);
if (p == nullptr) {
    Serial.println("malloc failed");
    return;
}

// calloc allocates and zero-initializes memory.
// Use it instead of malloc when zero initialization is required.

// Resize an existing allocation.
uint8_t *q = (uint8_t *)realloc(p, 200 * 1024);

if (q == nullptr) {
    // p is still valid: do not lose the original pointer.
    free(p);
    Serial.println("realloc failed");
    return;
}

p = q;
free(p);
```

Use a temporary pointer for `realloc()`. Assigning its return value directly to `p` can lose the original allocation if resizing fails.

`calloc()` initializes bytes to zero. `malloc()` leaves the contents uninitialized. Neither operation guarantees PSRAM unless the allocator configuration and requested capabilities provide it.

### 4.6 `ps_malloc`, `ps_calloc` and `ps_realloc`

These Arduino-ESP32 helpers are intended for PSRAM-oriented allocations.

```
uint8_t *a = (uint8_t *)ps_malloc(1024 * 1024);
uint8_t *b = (uint8_t *)ps_calloc(100 * 1024, 1);

if (a == nullptr || b == nullptr) {
    Serial.println("PSRAM allocation failed");
}

free(a);
free(b);
```

`ps_realloc()` is also available in applicable Arduino-ESP32 releases, but verify the installed Core 3.x headers if you intend to use it. Like any resize operation, it may move the allocation, and failure must be handled.

### 4.7 `String` versus `char[]` versus `std::vector`

| Type             | Strength                                            | Main risk / consideration                                                           |
| ---------------- | --------------------------------------------------- | ----------------------------------------------------------------------------------- |
| Arduino `String` | Convenient text manipulation                        | Repeated concatenation can cause allocation churn and fragmentation                 |
| `char[]`         | Predictable fixed capacity                          | Must enforce bounds and null termination                                            |
| `std::vector<T>` | Dynamic typed arrays, automatic lifetime management | Allocation follows its allocator; default vector storage is not automatically PSRAM |
| PSRAM allocation | Large buffers without consuming most internal heap  | Slower access and external-memory restrictions                                      |

For a large sensor history or image buffer, prefer an explicit PSRAM allocation or a custom allocator for a container. For a small fixed protocol field, a bounded `char[]` is often simpler.

## 5. Labs for Arduino-ESP32 Core 3.x

Before running the labs, check the board configuration.

### N16R8 preflight checklist

Board selected: ESP32-S3 Dev Module or the correct N16R8 board definition

Flash Size: 16 MB

PSRAM: OPI PSRAM

Partition Scheme: a compatible 16 MB layout

USB/Serial settings configured for your board and upload method

0 of 5 checks complete

For the user's N16R8 hardware, a successful initialization should report approximately 8,388,608 bytes of PSRAM and 16,777,216 bytes of Flash. If `psramFound()` returns `false`, do not assume the physical PSRAM is absent: first check that OPI PSRAM is enabled and the selected board configuration matches the hardware.

### Lab A — Compare internal heap and PSRAM

This sketch prints both heap types, then attempts allocations of 1 KB, 100 KB, 1 MB and 4 MB. It uses `malloc()` for the general heap test and `ps_malloc()` for the explicit PSRAM test.

The test retains each successful allocation until the end so you can observe how total free memory and the largest free block change.

```

#include <Arduino.h>
#include "esp_heap_caps.h"

struct Allocation {
  const char *label;
  void *ptr;
};

static Allocation allocations[8];
static size_t allocationCount = 0;

void printMemory(const char *tag) {
  Serial.printf("\n--- %s ---\n", tag);

  Serial.printf("Flash:           %u bytes\n",
                (unsigned)ESP.getFlashChipSize());
  Serial.printf("PSRAM detected:  %s\n",
                psramFound() ? "YES" : "NO");
  Serial.printf("PSRAM total:     %u bytes\n",
                (unsigned)ESP.getPsramSize());

  Serial.printf("Internal free:   %u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  Serial.printf("Internal largest:%u bytes\n",
                (unsigned)heap_caps_get_largest_free_block(
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

  Serial.printf("PSRAM free:      %u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  Serial.printf("PSRAM largest:   %u bytes\n",
                (unsigned)heap_caps_get_largest_free_block(
                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  Serial.printf("Default free:    %u bytes\n",
                (unsigned)ESP.getFreeHeap());
  Serial.printf("Default max alloc:%u bytes\n",
                (unsigned)ESP.getMaxAllocHeap());
  Serial.printf("PSRAM max alloc: %u bytes\n",
                (unsigned)ESP.getMaxAllocPsram());
}

void attempt(const char *label, size_t bytes, bool usePsram) {
  void *p = usePsram ? ps_malloc(bytes) : malloc(bytes);

  Serial.printf("%s %u bytes: %s\n",
                label, (unsigned)bytes,
                p ? "SUCCESS" : "FAILED");

  if (p && allocationCount < 8) {
    allocations[allocationCount++] = {label, p};
  } else if (p) {
    free(p);
  }

  printMemory(label);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\nESP32-S3 N16R8 memory lab");
  printMemory("START");

  const size_t sizes[] = {
    1024, 100 * 1024, 1024 * 1024, 4 * 1024 * 1024
  };

  for (size_t n : sizes) {
    attempt("malloc", n, false);
  }

  for (size_t n : sizes) {
    attempt("ps_malloc", n, true);
  }

  Serial.println("\nAll tests finished. Allocations retained.");
  printMemory("FINAL");
}

void loop() {
  delay(1000);
}

```

Expected observations

- Flash should report `16777216` bytes.
- PSRAM should report approximately `8388608` bytes.
- The initial internal free heap may be around a few hundred KB, depending on the runtime.
- A 1 MB or 4 MB general `malloc()` may fail, or may succeed from PSRAM if the active allocator policy permits it. Do not use the result alone to infer where the allocation was placed.
- `ps_malloc()` explicitly tests PSRAM. The 4 MB allocation may succeed when a sufficiently large contiguous PSRAM block is available.
- As allocations remain live, the free total and largest block should decrease.

To determine the location of a general `malloc()` allocation, use the heap-capability APIs or inspect the pointer with the relevant ESP-IDF memory-region facilities. The label `malloc` does not by itself prove that the allocation came from internal RAM.

### Lab B — Measure internal RAM versus PSRAM copy speed

The following test compares copying a 1 MB buffer in internal RAM against copying a 1 MB buffer in PSRAM. It uses `heap_caps_malloc()` to make placement explicit and a volatile checksum to keep the copied data observable.

A 1 MB internal allocation is normally too large for the available internal heap on this module. Therefore, the internal test uses a smaller 64 KB buffer by default; the PSRAM test uses 1 MB. To compare equal-sized buffers, choose a smaller size that fits in internal RAM and use that same size for both.

```

#include <Arduino.h>
#include "esp_heap_caps.h"

static void runCopyTest(const char *name, uint8_t *src,
                        uint8_t *dst, size_t bytes) {
  for (size_t i = 0; i < bytes; ++i) {
    src[i] = (uint8_t)(i * 17U);
  }

  // Warm up caches and memory paths.
  memcpy(dst, src, bytes);

  const uint32_t start = micros();

  for (int i = 0; i < 10; ++i) {
    memcpy(dst, src, bytes);
  }

  const uint32_t elapsed = micros() - start;

  volatile uint8_t checksum = 0;
  for (size_t i = 0; i < bytes; i += 4096) {
    checksum ^= dst[i];
  }

  const float totalMB = (float)bytes * 10.0f / 1000000.0f;
  const float seconds = elapsed / 1000000.0f;
  const float mbps = seconds > 0 ? totalMB / seconds : 0;

  Serial.printf("%s: %u bytes x 10, %u us, %.2f MB/s, checksum=%u\n",
                name, (unsigned)bytes, (unsigned)elapsed,
                mbps, (unsigned)checksum);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!psramFound()) {
    Serial.println("PSRAM not found. Check OPI PSRAM configuration.");
    return;
  }

  const size_t internalBytes = 64 * 1024;
  const size_t psramBytes = 1024 * 1024;

  uint8_t *isrc = (uint8_t *)heap_caps_malloc(
      internalBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  uint8_t *idst = (uint8_t *)heap_caps_malloc(
      internalBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  uint8_t *psrc = (uint8_t *)heap_caps_malloc(
      psramBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  uint8_t *pdst = (uint8_t *)heap_caps_malloc(
      psramBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!isrc || !idst || !psrc || !pdst) {
    Serial.println("Allocation failed.");
    free(isrc);
    free(idst);
    free(psrc);
    free(pdst);
    return;
  }

  Serial.println("Copy speed test (placement is explicit)");
  runCopyTest("Internal 64 KB", isrc, idst, internalBytes);
  runCopyTest("PSRAM 1 MB", psrc, pdst, psramBytes);

  free(isrc);
  free(idst);
  free(psrc);
  free(pdst);
}

void loop() {}

```

Interpret the result carefully: the two tests above use different buffer sizes, so they demonstrate real-world throughput at different scales, not a clean 1:1 speed ratio. For a fair comparison, set both sizes to 32 KB or another size that fits in internal RAM, then repeat with PSRAM. Large sequential copies can be several times slower in PSRAM depending on clock configuration, cache effects, compiler optimizations and contention. Treat “3–4× slower” as a possible measurement, not a hardware guarantee.

### Lab C — Monitor FreeRTOS task stack usage

The stack high-water mark reports the minimum remaining stack space observed by a task since it started. On ESP-IDF FreeRTOS, the returned value is in stack units; for ESP32-S3 the unit is normally bytes because `StackType_t` is 32 bits, but confirm the exact FreeRTOS port and version.

```

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t workerHandle = nullptr;

void workerTask(void *parameter) {
  for (;;) {
    // Simulate local data usage. Keep this modest.
    volatile uint32_t scratch[128];

    for (size_t i = 0; i < 128; ++i) {
      scratch[i] = (uint32_t)i * 3U;
    }

    Serial.printf("[Worker] stack high-water mark: %u units\n",
                  (unsigned)uxTaskGetStackHighWaterMark(nullptr));

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ESP-IDF / Arduino-ESP32 xTaskCreate stack depth is specified
  // in bytes in this ESP-IDF configuration.
  BaseType_t result = xTaskCreate(
      workerTask,
      "MemoryWorker",
      4096,
      nullptr,
      1,
      &workerHandle
  );

  if (result != pdPASS) {
    Serial.println("Failed to create worker task");
  }
}

void loop() {
  Serial.printf("[Loop] free internal heap: %u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  delay(3000);
}

```

Expected output is a repeating high-water mark and free-heap reading. Exact numbers depend on compiler, logging, core version and runtime load. If the high-water mark gets very small during realistic worst-case operation, increase the stack allocation or reduce stack use. A high-water mark is a diagnostic, not a guarantee against every possible future call path.

Do not put a large array on a task stack just because the module has 8 MB PSRAM. The normal `xTaskCreate()` stack is allocated internally by default. Stack placement in PSRAM requires specific ESP-IDF configuration and has cache-disabled and low-power restrictions. For general Arduino sketches, keep task stacks internal and move large data buffers to PSRAM.

## 6. ESP-IDF configuration and allocation reference

For an ESP-IDF project targeting the ESP32-S3 N16R8, enable Octal PSRAM and use a partition table sized for the 16 MB Flash device.

### 6.1 `sdkconfig.defaults`

```
# External PSRAM
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y

# Add PSRAM to the capability-based heap allocator
CONFIG_SPIRAM_USE_MALLOC=y

# Custom 16 MB partition table
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

These settings are appropriate starting points for ESP-IDF releases that expose these option names. Configuration symbols can change between ESP-IDF releases, so verify them against the version used by the project. In Arduino-ESP32, select the correct board and OPI PSRAM menu settings rather than copying `sdkconfig.defaults` into the sketch.

### 6.2 Capability-based allocation in ESP-IDF

```
#include <stdio.h>
#include <stdlib.h>
#include "esp_heap_caps.h"

void app_main(void)
{
    size_t internal_free =
        heap_caps_get_free_size(
            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    size_t psram_free =
        heap_caps_get_free_size(
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    printf("Internal free: %u bytes\n",
           (unsigned)internal_free);

    printf("PSRAM free: %u bytes\n",
           (unsigned)psram_free);

    // Large writable buffer in PSRAM
    uint8_t *image = heap_caps_malloc(
        1024 * 1024,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (image == NULL) {
        printf("PSRAM allocation failed\n");
        return;
    }

    image[0] = 0x5A;
    printf("First byte: 0x%02X\n", image[0]);

    free(image);
}
```

Include `<stdint.h>` in this example if it is not already included by another header, because the code uses `uint8_t`.

Use `heap_caps_malloc()` when the memory type is a design requirement. Use ordinary `malloc()` for general allocations where the selected heap policy is acceptable.

## 7. Three common N16R8 mistakes

**Mistake 1 — Forgetting OPI PSRAM**

The module physically contains 8 MB PSRAM, but the firmware must initialize it correctly. If `psramFound()` returns `false`, check the board definition, PSRAM mode and startup logs. Do not treat the physical capacity as proof that the runtime has initialized it.

**Mistake 2 — Using a 4 MB partition scheme**

The 16 MB Flash device does not override the selected partition table. A 4 MB layout can limit the application slot and leave most of the physical Flash unavailable to your chosen app/filesystem layout. Select or define a compatible 16 MB partition table.

**Mistake 3 — Putting every buffer in PSRAM**

PSRAM is excellent for large data arrays, but not every driver, DMA operation, ISR or cache-disabled execution path can use it. Allocate conservative DMA buffers with `MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA`; follow the specific ESP32-S3 peripheral driver's documented requirements.

## 8. Five-question quiz

1\. What is the nominal internal SRAM capacity of the ESP32-S3 N16R8?

- 16 MB
- 8 MB
- 512 KB
- 24 MB

2\. What determines the maximum firmware image size?

- The PSRAM capacity
- The selected application partition size
- The total Flash size alone
- The task stack size

3\. Which allocation is the conservative choice for a peripheral DMA buffer?

- ps_malloc(4096)
- A large local stack array
- heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)
- A const array in Flash

4\. What does a largest-free-block measurement tell you?

- Total physical RAM capacity
- The largest single contiguous suitable allocation
- The Flash partition size
- The stack high-water mark

5\. What does psramFound() returning false mean?

- The module definitely has no PSRAM chip
- The Flash is full
- PSRAM was not successfully detected or initialized
- The internal SRAM is exhausted

**Check answers**
1. 512 KB
2. The selected application partition size
3. heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)
4. The largest single contiguous suitable allocation
5. PSRAM was not successfully detected or initialized


## Final engineering checklist

- Flash: verify `ESP.getFlashChipSize()` returns `16777216`.
- PSRAM: verify `psramFound()` is true and `ESP.getPsramSize()` reports approximately `8388608`.
- Partitions: use a compatible 16 MB partition table and ensure the app image fits its selected slot.
- Heap: monitor total free bytes and largest allocatable block for both internal memory and PSRAM.
- Stack: use `uxTaskGetStackHighWaterMark()` during realistic worst-case task execution.
- DMA: request internal DMA-capable memory unless your exact peripheral driver and configuration explicitly support the intended PSRAM buffer.
- Performance: measure with your own workload; PSRAM's usable capacity is its major advantage, not SRAM-equivalent latency.

The key design principle is to use internal SRAM for critical, latency-sensitive and capability-constrained operations; PSRAM for large runtime data; and Flash for firmware and persistent storage.


