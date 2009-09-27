
DIRS = libs/ctaocrypt-1.1.0 libs/cyassl-1.1.0 src

all:	cleanlibs
	for i in $(DIRS); do $(MAKE) -C $$i || exit -1; done
	mv src/Anki.nds .

clean:  cleanlibs
	for i in $(DIRS); do $(MAKE) -C $$i clean || exit -1; done
		
dist:
	for i in $(DIRS); do $(MAKE) -C $$i dist || exit -1; done

cleanlibs:
	rm -f *.a *.bin *.elf *.img *.o
