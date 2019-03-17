NHWENC = ../encoder/nhwenc
NHWDEC = ../decoder/nhwdec

# Default setting:
SETTING = norm

DIFFTOOL = python analysis.py
# DIFFTOOL = meld

ALL_SETTINGS = h3 h1 norm l1 l3 l7 l9 l16

WORK_ROOT ?= $(HOME)/work

TESTDIR = $(WORK_ROOT)/nhw/src
WORKDIR = $(WORK_ROOT)/nhw/out
REFDIR =  $(WORK_ROOT)/nhw/ref

IMAGE_FILES = $(wildcard $(TESTDIR)/*.png)

IMAGES = $(patsubst $(TESTDIR)/%.png,%,$(IMAGE_FILES))

NHWIMAGES = $(IMAGES:%=$(WORKDIR)/%_$(SETTING).nhw)

LOOPBACK_IMAGES = $(IMAGES:%=$(WORKDIR)/%_$(SETTING).png)

$(WORKDIR)/%_$(SETTING).nhw: $(TESTDIR)/%.png
	$(NHWENC) $< -$(SETTING)
	mv $(TESTDIR)/$*.nhw $@

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

test: $(WORKDIR)/$(TESTIMAGE)_$(SETTING).nhw

reference:
	cp -r $(WORKDIR)/*.nhw $(REFDIR)

testdiff:
	@echo "=================================================="
	@for i in $(ALL_SETTINGS); do \
		diff $(WORKDIR)/$(TESTIMAGE)_$$i.nhw \
		$(REFDIR)/$(TESTIMAGE)_$$i.nhw; \
	done

hexdiff: $(WORKDIR)/$(TESTIMAGE)_$(SETTING).hex \
		$(REFDIR)/$(TESTIMAGE)_$(SETTING).hex
		meld $^

diff:
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

clean:
	rm -f $(WORKDIR)/*.nhw

nhw: $(NHWIMAGES)
