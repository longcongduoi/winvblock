rem This is the root directory of the ddk.
set ddkdir=c:\winddk\6001.18001

rem Next two lines are duplicated in Makefile, edit both when adding files or changing pxe style.
set c=driver.c registry.c bus.c aoedisk.c aoe.c protocol.c debug.c probe.c
set pxestyle=asm
