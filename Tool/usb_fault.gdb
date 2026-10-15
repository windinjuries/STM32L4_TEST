#
# usb_fault.gdb - decode the USB Audio/CDC composite HardFault
#
# Usage (from the GDB console of STM32CubeIDE / CubeCLT / OpenOCD):
#     source D:/folder/code/STM32L4_TEST/Tool/usb_fault.gdb
#
# Run it while the core is halted in HardFault_Handler. Everything is printed
# with simple commands only; there are no nested quotes to upset the IDE
# console.
#

echo \n===== FAULT DECODE =====\n
printf "stacked PC : 0x%08x\n", hf_stacked_pc
printf "stacked LR : 0x%08x\n", hf_stacked_lr
printf "stacked PSR: 0x%08x\n", hf_stacked_psr
printf "r0         : 0x%08x\n", hf_stacked_r0
printf "r1         : 0x%08x\n", hf_stacked_r1
printf "r2         : 0x%08x\n", hf_stacked_r2
printf "r3         : 0x%08x\n", hf_stacked_r3
printf "CFSR       : 0x%08x\n", hf_cfsr
printf "HFSR       : 0x%08x\n", hf_hfsr
printf "BFAR       : 0x%08x\n", hf_bfar
printf "MMFAR      : 0x%08x\n", hf_mmfar
printf "EXC_RETURN : 0x%08x\n", hf_exc_return
printf "captured MSP=0x%08x  PSP=0x%08x\n", hf_msp, hf_psp

echo \n--- CFSR decode ---\n
set $c = hf_cfsr
if ($c & (1 << 25))
  echo DIVBYZERO\n
end
if ($c & (1 << 24))
  echo UNALIGNED access fault\n
end
if ($c & (1 << 16))
  echo UNDEFINSTR (executed an undefined instruction)\n
end
if ($c & (1 << 17))
  echo INVSTATE (bad Thumb state -> branched to non-code)\n
end
if ($c & (1 << 18))
  echo INVPC\n
end
if ($c & (1 << 19))
  echo NOCP (coprocessor access)\n
end
if ($c & (1 << 8))
  echo IBUSERR (instruction fetch failed)\n
end
if ($c & (1 << 9))
  echo PRECISERR (precise data access fault)\n
end
if ($c & (1 << 10))
  echo IMPRECISERR (imprecise data access fault)\n
end
if ($c & (1 << 15))
  printf "BFARVALID=1 -> faulting address = 0x%08x\n", hf_bfar
else
  echo BFARVALID=0 -> address not reported by the bus\n
end

# The exception frame the hardware pushed. hf_msp was captured at handler
# entry, before the compiler emitted any prologue, so frame = msp + 32.
echo \n--- exception frame (as pushed by hardware) ---\n
set $ef = (unsigned int *)(hf_msp + 32)
printf "R0 =0x%08x  R1 =0x%08x  R2 =0x%08x  R3 =0x%08x\n", $ef[0], $ef[1], $ef[2], $ef[3]
printf "R12=0x%08x  LR =0x%08x  PC =0x%08x  PSR=0x%08x\n", $ef[4], $ef[5], $ef[6], $ef[7]

echo \n===== USB CLASS STATE =====\n
printf "classId            = %u\n", hUsbDeviceFS.classId
printf "NumClasses         = %u\n", hUsbDeviceFS.NumClasses
printf "pUserData[0]       = 0x%08x   expect 0x20000138 (USBD_AUDIO_fops_FS)\n", hUsbDeviceFS.pUserData[0]
printf "pUserData[1]       = 0x%08x   expect 0x2000015c (USBD_CDC_fops_FS)\n", hUsbDeviceFS.pUserData[1]
printf "pClassDataCmsit[0] = 0x%08x   audio handle\n", hUsbDeviceFS.pClassDataCmsit[0]
printf "pClassDataCmsit[1] = 0x%08x   cdc handle\n", hUsbDeviceFS.pClassDataCmsit[1]
printf "pClass[0]          = 0x%08x   expect USBD_CMPSIT\n", hUsbDeviceFS.pClass[0]
printf "pClass[1]          = 0x%08x   expect USBD_AUDIO\n", hUsbDeviceFS.pClass[1]
printf "pClass[2]          = 0x%08x   expect USBD_CDC\n", hUsbDeviceFS.pClass[2]
printf "usbd_static_used   = %u\n", usbd_static_used

echo \n--- fops tables, first 6 words (word0 must be a 0x0800xxxx code address) ---\n
echo USBD_AUDIO_fops_FS:\n
x/6xw &USBD_AUDIO_fops_FS
echo USBD_CDC_fops_FS:\n
x/6xw &USBD_CDC_fops_FS

echo \n--- static pool region (first 8 words; must not be zeroed by CDC) ---\n
x/8xw &usbd_static_pool

echo \n===== END =====\n
