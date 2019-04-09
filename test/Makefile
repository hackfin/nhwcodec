NHWENC = ../encoder/nhwenc
NHWDEC = ../decoder/nhwdec

-include ../encoder/config.mk

SWAP ?= y

# Default setting:
SETTING = norm

DIFFTOOL = python analysis.py
# Image comparison:
CMP = compare -fuzz 1 -metric rmse
# DIFFTOOL = meld

ALL_SETTINGS = h3 h2 h1 norm l1 l3 l5 l6 l7 l9 l8 l12  l16

WORK_ROOT ?= $(HOME)/work

TESTDIR = $(WORK_ROOT)/nhw/src
WORKDIR = $(WORK_ROOT)/nhw/out
REFDIR =  $(WORK_ROOT)/nhw/ref

IMAGE_FILES = $(wildcard $(TESTDIR)/*.png)

IMAGES = $(patsubst $(TESTDIR)/%.png,%,$(IMAGE_FILES))

NHWIMAGES = $(IMAGES:%=$(WORKDIR)/%_$(SETTING).nhw)

LOOPBACK_IMAGES = $(IMAGES:%=$(WORKDIR)/%_$(SETTING).png)

# Default tile power
POWER = 6

ifdef TEST
TEST_MODE = -T
endif

ifdef SWAP
NHW_ENCODE = $(NHWENC) $< -$(SETTING) -o $@ $(TEST_MODE)
else
NHW_ENCODE = $(NHWENC) $< -$(SETTING); mv $(TESTDIR)/$*.nhw $@
endif

$(WORKDIR)/%_$(SETTING).nhw: $(TESTDIR)/%.png
	$(NHW_ENCODE)

%.png: %.nhw
	$(NHWDEC) $<

nhw: $(NHWIMAGES)

HEXFILES_OUT = $(IMAGES:%=$(WORKDIR)/%_$(SETTING).hex)
HEXFILES_REF = $(IMAGES:%=$(REFDIR)/%_$(SETTING).hex)

%.hex: %.nhw
	hexdump -C $< >$@

hexfiles: $(HEXFILES_REF) $(HEXFILES_OUT)

show:
	@echo $(NHWIMAGES)
	@echo $(IMAGES)

loopback: $(LOOPBACK_IMAGES)

all:
	@for i in $(ALL_SETTINGS); do \
		$(MAKE) loopback SETTING=$$i; \
	done


all-%:
	@for i in $(ALL_SETTINGS); do \
		$(MAKE) $* SETTING=$$i; \
	done

TESTIMAGE = will-b-272731-unsplash

reference:
	git describe >> $(REFDIR)/GIT_REVISION
	cp -r $(WORKDIR)/*.nhw $(REFDIR)
	cp -r $(WORKDIR)/*.png $(REFDIR)


REFIMAGES = $(IMAGES:%=$(REFDIR)/%_$(SETTING)DEC.png)
CURIMAGES = $(IMAGES:%=$(WORKDIR)/%_$(SETTING)DEC.png)

EXTENSION = _$(SETTING)DEC

compare:
	@echo "=================================================="
	for i in $(IMAGES); do \
		DIFFIMAGES="$(WORKDIR)/$${i}$(EXTENSION).png \
		$(REFDIR)/$${i}$(EXTENSION).png" ; \
		diff $$DIFFIMAGES || \
		( $(CMP) $$DIFFIMAGES /tmp/diff_$${i}$(EXTENSION).png; \
			display /tmp/diff_$${i}$(EXTENSION).png ) & \
	done

pngdiff: $(CURIMAGES)
	@echo ============= Quality '$(SETTING)' ===============
	@for i in $(IMAGES); do \
		diff $(WORKDIR)/$${i}$(EXTENSION).png \
	$(REFDIR)/$${i}$(EXTENSION).png || ERR=1 ;\
	done; \
	echo "==================================================" ; \
	if [ "$$ERR" == "1" ]; then echo "FAIL"; false; else true; fi


testdiff: $(NHWIMAGES)
	@echo "=================================================="
	@for i in $(IMAGES); do \
		diff $(WORKDIR)/$${i}_$(SETTING).nhw \
		$(REFDIR)/$${i}_$(SETTING).nhw; \
	done

hexdiff: $(WORKDIR)/$(TESTIMAGE)_$(SETTING).hex \
		$(REFDIR)/$(TESTIMAGE)_$(SETTING).hex
		meld $^


diff: $(WORKDIR)/$(TESTIMAGE)_$(SETTING).nhw
	@echo ============= Quality '$(SETTING)' ===============
	@diff $(WORKDIR)/$(TESTIMAGE)_$(SETTING).nhw \
		$(REFDIR)/$(TESTIMAGE)_$(SETTING).nhw || \
	$(DIFFTOOL) \
		$(REFDIR)/$(TESTIMAGE)_$(SETTING).nhw \
		$(WORKDIR)/$(TESTIMAGE)_$(SETTING).nhw
	@echo ===============  DONE  ==================

%-out.txt: $(WORKDIR)/%.nhw
	python analysis.py $< >$@
	
%-ref.txt: $(REFDIR)/%.nhw
	python analysis.py $< >$@
	
ana-%:
	$(MAKE) $*-out.txt $*-ref.txt
	meld $*-out.txt $*-ref.txt

allhex:
	@for i in $(ALL_SETTINGS); do \
		$(MAKE) hexfiles SETTING=$$i; \
	done

$(WORKDIR)/%_t$(POWER)_$(SETTING).png: $(TESTDIR)/%.png
	@echo ============= Quality '$(SETTING)' ===============
	$(NHW_ENCODE) -LT -s$(POWER) $(SETTING) $<
	@echo ===============  DONE  ==================

TILE_IMAGES = $(IMAGES:%=$(WORKDIR)/%_t$(POWER)_$(SETTING).png)

tile: $(TILE_IMAGES)

VALGRIND_OPTS = --track-origins=yes --leak-check=full --error-exitcode=1
VALGRIND_OPTS += --read-var-info=yes

valgrind: $(TESTDIR)/$(TESTIMAGE).png
	@echo ============= Quality '$(SETTING)' ===============
	valgrind $(VALGRIND_OPTS) \
		--read-var-info=yes \
	$(NHWENC) $< -$(SETTING) 
	@echo ===============  DONE  ==================


clean::
	rm -f $(TESTDIR)/*.nhw
	rm -f $(WORKDIR)/*.nhw $(WORKDIR)/*.hex $(REFDIR)/*.hex 

nhw: $(NHWIMAGES)

test:
	

verify: all-nhw all-testdiff
