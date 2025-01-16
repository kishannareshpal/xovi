.global untrampolineFunction
.type untrampolineFunction, %function
.global untrampolineStep2
.type untrampolineStep2, %function

.global untrampolineInit
.type untrampolineInit, %function
.global untrampolineFini
.type untrampolineFini, %function

untrampolineFunction:
    # According to the program, we're in the 'library function'.
    # All argument registers should be considered unsafe to work on. They need to be saved
    # Back up r0-r3
    # In r12 there is our symbol_data structure.

    # Backup frame pointer and link register:
    stmfd sp!, {r11, lr}
    # Backup arguments and the info we need to continue
    stmfd sp!, {r0-r3, r12}
    # All registers backed up! We can now call the function (re)constructor in C
    # It needs the symbol_data struct ptr
    mov r0, r12
    bl untrampolineInit
    # Now it's safe to say all are registers are destroyed
    # Reconstruct...
    ldmfd sp, {r0-r3, r12}
    
    # Now it's safe to jump to the function
    ldr r12, [r12]
    blx r12
    # Store return value
    str r0, [sp, #16]
    # This is a call - this function will return.
    # Restore the registers we need to make the function a trampoline again
    ldr r0, [sp]
    bl untrampolineFini
    # We can now restore everything
    ldmfd sp!, {r0-r3, r12}
    ldmfd sp!, {r11, lr}
    bx lr
# End

untrampolineStep2:
    # After we enter the function, we need to get rid of the original code
    # as soon as possible.
    # Right now we have just barely started executing the function (4 instrs done)
    # Replace the beginning of the function to the original trampoline.
    # Then replace the second trampoline to original code. Do so WITHOUT USING ANY
    # REGISTERS. NONE OF THEM ARE SAFE AT THIS POINT.
    stmfd sp!, {r0-r3}
    mrs r0, cpsr
    stmfd sp!, {r0, r12}
    # I will need at least the 4 registers to implement a copy loop.
    # r0 - destination,
    # r1 - source
    ldr r0, [r12]
    ldr r1, [r12, #12]
    # Copy S1 Trampoline --> Start of function
    mov r2, #3
    .l1:
    ldr r3, [r1], #4
    str r3, [r0], #4
    subs r2, r2, #1
    bne .l1
    # After 3 copies, we're restored
    # Now copy original code (f) into the current address
    ldr r1, [r12, #8]
    add r1, r1, #12
    mov r2, #4
    .l2:
    ldr r3, [r1], #4
    str r3, [r0], #4
    subs r2, r2, #1
    bne .l2
    # Copied. Prepare jump to address[3] (12) - wipe the CPU cache
    ldmfd sp, {r12}
    ldr r12, [r12]
    
    # Cache clearing operations that I do not know the ARMv7 equivalent of and whether they are necessary
    #mov r0, #8
    #isb
    #dsb sy

    #bic r12, r12, #0x3F
    #.l3:
    #dc cvau, x17
    #dsb ish
    #ic ivau, x17
    #dsb ish
    #isb sy
    #add x17, x17, #64
    #isb
    #dsb sy
    #subs x0, x0, #1
    #bne .l3
    
    ldmfd sp!, {r0, r12}
    msr cpsr, r0
    ldmfd sp!, {r0-r3}
    
    ldr r12, [r12]
    # Done. Return to the function.
    add r12, r12, #12
    bx r12
