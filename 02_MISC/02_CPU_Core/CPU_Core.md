# ESP32-S3 Dual-Core Architecture: How the Two Cores Work Together

The ESP32-S3 in your ESP32-S3 DevKitC-1 N16R8 is a dual-core Xtensa LX7 microcontroller. Its two CPU cores can execute different instructions simultaneously, share access to memory and peripherals, and communicate through shared memory and synchronization mechanisms.

One important correction to your observation:

> It is not true that only one core is used at a time. Both cores can execute code concurrently. However, a simple Arduino sketch may appear to use only one core because Arduino's main `setup()` and `loop()` task normally run on Core 1, while Core 0 runs other system tasks, such as Wi-Fi and Bluetooth-related work. The actual task placement depends on the ESP-IDF/Arduino-ESP32 version and configuration.

## 1. The overall architecture

[embeddedcomputing.com](https://embeddedcomputing.com/technology/ai-machine-learning/espressif-providing-support-for-the-aiot-market-with-its-dual-core-esp32-s3)

<img width="977" height="966" alt="image" src="https://github.com/user-attachments/assets/785f1a04-9d3b-422c-b557-d941c3d448c2" />


Conceptually, the architecture looks like this:

```text
CPU Core 0
Xtensa LX7
Executes assigned tasks

CPU Core 1
Xtensa LX7
Executes assigned tasks

On-chip interconnect and memory system
Coordinates access to shared resources

Shared memory
SRAM, ROM, external PSRAM interface

Shared peripherals
GPIO, timers, SPI, I²C, UART, Wi-Fi subsystem
```

This is a conceptual diagram rather than a detailed electrical bus diagram. The actual chip contains several bus paths, memory controllers, caches, DMA-capable peripherals and hardware-specific access restrictions.

### Core specifications

| Feature | ESP32-S3 |
|---|---|
| CPU architecture | Xtensa LX7, 32-bit |
| CPU cores | 2 |
| Maximum CPU frequency | 240 MHz |
| Instruction execution | Independent on each core |
| Hardware floating point | Single-precision FPU per core |
| SIMD/DSP acceleration | Available through Xtensa instructions |
| Memory model | Shared address space, subject to access and cache rules |
| Task scheduling | FreeRTOS in the Arduino-ESP32 environment |

At 240 MHz, each core can execute up to 240 million clock cycles per second. That does not mean every instruction completes in one cycle, or that the complete system has exactly twice the performance of a single-core device.

## 2. What each core contains—and what it shares

The two cores are separate execution engines, not two independent microcontrollers.

| Resource | Core 0 | Core 1 | How it works |
|---|---|---|---|
| Registers and program counter | Private | Private | Each core maintains its own execution state. |
| General-purpose arithmetic and instruction execution | Private execution unit | Private execution unit | Both can execute instructions at the same time. |
| Interrupt handling state | Per-core | Per-core | Each core handles its own CPU interrupt state. |
| Internal SRAM | Shared address space | Shared address space | Both can access shared memory, subject to hardware access rules. |
| Flash-resident program code | Shared address space | Shared address space | Both can execute the same firmware image. |
| External PSRAM | Shared address space when mapped and accessible | Shared address space when mapped and accessible | Both can use accessible PSRAM, but cache and memory constraints matter. |
| GPIO and common peripherals | Accesses common hardware | Accesses common hardware | A peripheral is not duplicated just because there are two cores. |
| FreeRTOS scheduler | Participates | Participates | The scheduler coordinates which task runs on which core. |

The cores generally see a common memory address space, but this does not mean every memory region is equally accessible in every execution context. Some regions have special restrictions, and memory-mapped flash/PSRAM access involves cache and memory-controller behavior. Espressif describes the ESP32-S3 as an SMP-capable dual-core target with shared memory and cross-core interrupts.

## 3. How the cores communicate

There is no need to connect Core 0 to Core 1 using UART, SPI, I²C or another external interface. They are on the same chip and can communicate using several mechanisms.

### A. Shared memory

Core 0 writes data into an accessible RAM location; Core 1 reads it. Both cores can access the same global variable, buffer or data structure.

**Example:** Core 0 receives sensor samples and stores them in a shared buffer. Core 1 processes the samples and updates the display.

**Important:** shared memory provides a place to exchange data, but does not automatically make simultaneous access safe.

### B. FreeRTOS queues, semaphores and task notifications

These provide controlled ways to pass data or signal events. A queue can copy a small message from one task to another; a semaphore can signal that a resource or event is available; a task notification can wake a task.

These mechanisms are often the easiest and safest way to coordinate application tasks.

### C. Cross-core interrupts

Hardware allows one core to interrupt or signal the other. The operating system can use this capability to request scheduling activity or coordinate inter-processor operations.

Most Arduino application code should use FreeRTOS APIs rather than implement cross-core interrupts directly.

### D. Locks and atomic operations

Mutexes, critical sections, spinlocks and atomic operations help protect data that both cores may access. ESP-IDF's SMP implementation supports hardware atomic compare-and-swap and serializes conflicting shared-memory accesses at the memory bus level.

Bus serialization prevents conflicting transactions from occurring as if they were simultaneous; it does not automatically protect a multi-step operation such as `counter++`.

### Example: why shared variables need protection

Suppose both cores execute this operation at the same time:

```cpp
counter++;
```

This is logically a read-modify-write sequence:

1. Read `counter`.
2. Add one.
3. Write the result back.

If both cores read `counter == 10` before either writes, they could both write `11`. One increment is lost.

For shared counters, use an appropriate atomic operation or protect the update with a synchronization mechanism. For more complex data structures, use a mutex or a queue where appropriate.

## 4. How the two cores operate together under Arduino-ESP32

Your Arduino IDE setup uses Arduino-ESP32 Core 3.3.12, built on ESP-IDF and FreeRTOS. The scheduler runs tasks on the two cores, and each core can execute a different task at the same time.

### Example task distribution

| Core | Example workload |
|---|---|
| Core 0 | Wi-Fi protocol processing, network event handling, background system tasks |
| Core 1 | Arduino `loop()` task, sensor processing, display updates |

Typical arrangement, not a guarantee that every task is pinned to that core. Actual placement depends on the framework version, configuration and task affinity.

- [FreeRTOS (IDF)](https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/system/freertos_idf.rst)
- [Expressif FreeRTOS (Overview)](https://docs.espressif.com/projects/esp-idf/en/v5.0.9/esp32s3/api-reference/system/freertos.html)

The scheduler does not normally run the same task on both cores simultaneously. A task is scheduled on one eligible core at a time, while other tasks can run on the other core.

For example, a Wi-Fi task could be processing incoming packets on Core 0 while your application task computes a temperature average on Core 1. Both execute concurrently, although they may contend for memory bandwidth, locks or other shared resources.

### Arduino `setup()` and `loop()`

A standard sketch such as:

```cpp
void setup() {
  Serial.begin(115200);
}

void loop() {
  // Application work
  delay(1000);
}
```

does not explicitly create two application tasks. Arduino runs the sketch through its main task, which is typically pinned to Core 1 in the ESP32 Arduino framework. The other core is still active, running RTOS and system tasks as needed.

The `delay(1000)` call yields the CPU while the task waits. It does not mean Core 1 is switched off or that Core 0 must take over the same task.

### Create a task on each core

You can test dual-core execution directly:

```cpp
#include <Arduino.h>

void core0Task(void *parameter) {
  while (true) {
    Serial.printf("Task A running on Core %d\n",
                  xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void core1Task(void *parameter) {
  while (true) {
    Serial.printf("Task B running on Core %d\n",
                  xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  xTaskCreatePinnedToCore(
    core0Task, "Core0Task", 4096, nullptr, 1, nullptr, 0);

  xTaskCreatePinnedToCore(
    core1Task, "Core1Task", 4096, nullptr, 1, nullptr, 1);

  Serial.printf("setup/loop task core: %d\n", xPortGetCoreID());
}

void loop() {
  delay(1000);
}
```

Open Serial Monitor at 115200 baud. You should see the two tasks report different core IDs, interleaved with the main sketch's output.

This demonstrates that tasks can execute on both cores. The serial output itself is shared and may be interleaved, so it is not a precise timing measurement. Also, this example intentionally pins the tasks; it does not prove that every system task uses the same core assignments.

## 5. Memory architecture: SRAM, flash, PSRAM and caches

For your N16R8 board, memory sharing is particularly important because you have 16 MB flash and 8 MB PSRAM.

### Internal SRAM — 512 KB nominal on-chip SRAM

Fast working memory for program data, task stacks, heap allocations and other runtime needs. It is shared across the system, but some regions have specific purposes or accessibility constraints.

### Flash — 16 MB on your board

Stores firmware, OTA application images, partitions and persistent data. Both cores execute code from the same firmware image through the memory/cache system. Flash is not ordinary writable RAM; OTA writes a new application image to the inactive partition.

### PSRAM — 8 MB on your board

External RAM useful for large buffers, image data, display frame buffers and other allocations. Both cores can use accessible PSRAM, but it has different performance characteristics and restrictions from internal SRAM.

### Instruction and data caches

The ESP32-S3 uses shared instruction and data caches for the two cores. If both cores request cached external-memory data simultaneously, hardware arbitration determines which request proceeds first. Cache misses can add latency.

- [Chapter 4 System and Memory](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf)
- [4 Functional Descrition](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)

### Does each core get 8 MB of PSRAM?

No. Your board has 8 MB total PSRAM, not 8 MB per core. Both cores use the same physical memory resource, subject to memory mapping and access rules.

Similarly, the 16 MB flash is shared storage. It is not split into 8 MB per core.

There is also no guarantee that all 512 KB of internal SRAM is available to your application: the runtime, stacks, buffers, caches and other system functions consume memory, and some internal memory may be assigned to particular functions.

### What happens when both cores access memory simultaneously?

The memory system arbitrates competing requests. This ensures transactions are handled correctly, but simultaneous demand can increase latency. The cores are independent execution engines, yet they are not guaranteed independent memory bandwidth.

For example, if Core 0 processes a network buffer while Core 1 reads a large display frame buffer in PSRAM, both workloads may contend for access to external memory.

## 6. How shared peripherals work

The two cores share the chip's peripheral system. A UART, SPI controller, I²C controller or GPIO output is a hardware resource—not a separate copy for each core.

| Peripheral | Can both cores use it? | Important consideration |
|---|---|---|
| GPIO | Yes, through the shared GPIO hardware | Avoid conflicting writes to the same pin. |
| UART | Yes, through software and shared hardware | Concurrent access to one UART's driver or transmit stream must be coordinated. |
| SPI | Yes | A single SPI bus/controller must be managed correctly when multiple tasks use it. |
| I²C | Yes | Protect shared bus transactions and driver state. |
| LED PWM / MCPWM | Yes, through shared peripheral hardware | Coordinate channel and configuration ownership. |
| Wi-Fi | Both cores can run tasks related to networking | Wi-Fi hardware and its software stack are shared system resources, not one independent radio per core. |
| USB | Yes, where supported by the relevant USB controller and driver | Coordinate ownership of the USB device/host functionality and its driver. |

### Example: two tasks trying to use the same SPI bus

Imagine your ESP32-S3 has an ILI9488 TFT and an SD card sharing an SPI bus.

- Core 0's task tries to write a display command.
- Core 1's task tries to read a file from the SD card.

If both tasks operate the bus without proper coordination, chip-select signals, transactions or driver state can conflict. The solution is to use the SPI driver's locking and transaction mechanisms, or a mutex where appropriate.

The two cores do not automatically make SPI twice as fast, nor do they create a second physical SPI controller for free.

## 7. How to use both cores effectively

For a real application, think in terms of parallel tasks with clear responsibilities, not simply “put half the code on each core.”

For example, an IoT device could use this division:

### Core 0 — communications-oriented workload

- Wi-Fi-related system tasks
- Network events
- An application task handling incoming commands

### Core 1 — application-oriented workload

- Sensor sampling
- Control calculations
- Display updates
- User-interface logic

### Communication between application tasks

Use FreeRTOS queues, task notifications, mutexes or protected shared buffers according to the data and timing requirements.

This is only an example. Don't pin tasks unnecessarily: unpinned tasks may run on either core, while pinned tasks are restricted to the selected core. The scheduler considers priority, task affinity and whether a task is already running on the other core.

### Practical rules

- Start with one core's worth of application logic. The system already uses multiple tasks; adding your own tasks is not always necessary.
- Use queues for producer-consumer designs. They are often easier to reason about than manually shared buffers.
- Use mutexes for shared peripherals. Particularly for SPI, I²C and shared driver state.
- Keep time-critical work short. Avoid long blocking operations or busy loops that prevent other important tasks from running.
- Measure before optimizing. Dual-core execution improves throughput only when tasks can run concurrently without excessive contention or synchronization overhead.

- [FreeRTOS (IDF)](https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/system/freertos_idf.rst)
- [Expressif FreeRTOS (Overview)](https://docs.espressif.com/projects/esp-idf/en/v5.0.9/esp32s3/api-reference/system/freertos.html)



## 8. How to verify core activity on your board

The simplest test is to create one task pinned to Core 0 and another pinned to Core 1, as shown earlier. For more detailed investigation, you can also:

- Print `xPortGetCoreID()` from within each task.
- Inspect task information using FreeRTOS task-list and runtime-statistics facilities, when enabled.
- Use Espressif's tracing and performance-monitoring tools to examine scheduling and execution.

A task's reported core ID tells you where it is executing at that moment. It does not by itself tell you how much CPU time each core spends on your application versus system work.

## 9. Key takeaways

- The ESP32-S3 has two independent CPU execution cores capable of running concurrently.
- The cores communicate through shared memory, FreeRTOS synchronization mechanisms and cross-core interrupts.
- They share the same physical SRAM, flash, PSRAM and peripheral system, with some hardware-specific access restrictions.
- A shared bus or memory resource can become a bottleneck even when both cores are active.
- Arduino `setup()` and `loop()` do not automatically use both cores; explicit FreeRTOS tasks are one way to make your application perform work on both.

For the definitive hardware-level view, consult Espressif's [ESP32-S3 Technical Reference Manual](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf), especially the System and Memory chapter, and the [ESP-IDF FreeRTOS documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/system/freertos.html).
