
DIRS = ctaocrypt-1.1.0 cyassl-1.1.0 expat-2.0.1

all:	build

build:	buildlibs
	for i in $(DIRS); do $(MAKE) -C $$i || exit -1; done

clean:  cleanlibs
	for i in $(DIRS); do $(MAKE) -C $$i clean || exit -1; done
		

cleanlibs:
	rm -f lib/*.a

buildlibs:
	echo "building libs"