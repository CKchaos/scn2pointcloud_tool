#
# Makefile for GAPS apps
#

opt:
	$(MAKE) target "TARGET=$@"

debug:
	$(MAKE) target "TARGET=$@"

mesa:
	$(MAKE) target "TARGET=$@"

clean:
	$(MAKE) target "TARGET=$@"

release:
	mkdir -p ../release/apps
	cp Makefile ../release/apps
	$(MAKE) target "TARGET=$@"

target: 
	cd scn2scn; $(MAKE) $(TARGET)
	cd scn2img; $(MAKE) $(TARGET)
	cd scnview; $(MAKE) $(TARGET)








