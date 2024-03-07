# Pico External Memory Library

The Raspberry Pi Pico / RP2040 microcontroller is a wonderful uC, however it lacks one thing that many ESP32s and similar have - An External Memory Interface!
This library intends to provide this through several means, firstly a simple memory interface class for reading/writing data, secondly a hardfault handler that allows mapping a single memory interface to an address in memory.

## How it works

The RP2040 is built on an ARM Cortex M0+ which uses the V6m Architecture. When this processor accesses memory outside of any mapped memory/peripherals (or some other conditions as well that we won't go into here), it triggers a hardfault exception which can be caught!
By catching this hardfault, we can decode the offending instruction and emulate its execution and access to the given memory interface, and then gracefully return to the next instruction. To the programmer, there is now usable memory in that location! (Although it is not at all fast)

## Example

Here is an example of reading a single 32bit word from a mapped region at 0x3000_0000.
```cpp
uint32_t test() {
  return *((volatile uint32_t*)(0x3000'0000));
}
```

This generates the following assembly
```asm
test():
        mov     r3, #805306368
        ldr     r0, [r3]
        bx      lr
```

Upon execution of the `ldr r0, [r3]` the processor hardfaults and execution is handed over to the hardfault handler. When this returns, the next instruction is executed `bx lr` and the correct value has been put into r0.