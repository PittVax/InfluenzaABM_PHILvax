##################### PHIL Makefile ###########################

DIRS = bin input_files src populations
PHIL_HOME = $(CURDIR)

all:
	@for i in $(DIRS); do \
		echo $$i; \
		(cd $$i; make); \
	done

clean:
	@for i in $(DIRS); do \
		echo $$i; \
		(cd $$i; make clean); \
	done

VER = 2.1.0

release:
	make clean
	(cd ..; tar cvzf PHIL-V${VER}-`date +"%Y-%m-%d"`.tgz \
	--exclude CVS --exclude '*~' --exclude '\.*' \
	PHIL/Makefile PHIL/LICENSE PHIL/bin PHIL/input_files \
	PHIL/populations/2005_2009_ver2_42003.zip PHIL/populations/Makefile \
	PHIL/src)

tar: clean
	cd ..
	tar cvsf PHIL-`date +"%Y-%m-%d"`.tgz PHIL --exclude RESULTS




