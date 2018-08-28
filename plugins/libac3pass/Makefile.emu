CFLAGS=-g -Wall
LINE1=Digital L
LINE2=Digital R
SPCS=1
#LINE1=Rear L
#LINE2=Rear R
#SPCS=4

all: play-ac3 play-raw

load:	unload
	emu-config -t $(SPCS)
	emu-dspmgr -a"fx15:$(LINE1)"
	emu-dspmgr -a"fx15:$(LINE2)"
	emu-dspmgr -l "$(LINE1)" -l "$(LINE2)" -f /usr/local/share/emu10k1/ac3pass.bin

unload:
	emu-dspmgr -z
	emu-config -t 0
#	emu-dspmgr -r"fx15:$(LINE1)"
#	emu-dspmgr -r"fx15:$(LINE2)"
#	emu-dspmgr -l "$(LINE1)" -l "$(LINE2)" --unload=AC3pass

play-ac3: play-ac3.c ac3-iec958.o

play-raw: play-raw.c

clean:
	rm -f *.o play-ac3 play-raw
