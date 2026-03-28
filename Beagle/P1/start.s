.section .text.startup
.syntax unified
.code 32
.globl _start

_start:
    bl main

hang_process:
    b hang_process