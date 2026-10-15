set pagination off
set confirm off
set height 0
file D:/folder/code/STM32L4_TEST/build/Debug/TEST.elf
target extended-remote localhost:50000
monitor reset halt

break USBD_RegisterClassComposite
break USBD_CMPSIT_AddClass
break Error_Handler
echo \n=== RUN ===\n
continue
echo \n=== HIT A ===\n
bt 3
p classtype
continue
echo \n=== HIT B ===\n
bt 3
p class
continue
echo \n=== HIT C ===\n
bt 3
continue
echo \n=== HIT D ===\n
bt 3
continue
echo \n=== HIT E ===\n
bt 6
p/x $pc
continue
echo \n=== HIT F ===\n
bt 6
p/x $pc

detach
quit
