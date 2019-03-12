NHWENC = ../encoder/nhwenc
NHWDEC = ../decoder/nhwdec

SETTING = h1

ALL_SETTINGS = h3 l1 l3 l6 l7 l9 l16
TESTDIR = /tmp/nhw/src
WORKDIR = /tmp/nhw/out

IMAGE_FILES = $(wildcard $(TESTDIR)/*.png)

IMAGES = $(patsubst $(TESTDIR)/%.png,%,$(IMAGE_FILES))

NHWIMAGES = $(IMAGES:%=$(WORKDIR)/%_$(SETTING).nhw)

LOOPBACK_IMAGES = $(IMAGES:%=$(WORKDIR)/%_$(SETTING).png)

$(WORKDIR)/%_$(SETTING).nhw: $(TESTDIR)/%.png
	$(NHWENC) $< -$(SETTING)
	mv $(TESTDIR)/$*.nhw $@

%.png: %.nhw
	$(NHWDEC) $<

show:
	@echo $(NHWIMAGES)
	@echo $(IMAGES)

loopback: $(LOOPBACK_IMAGES)

all:
	@for i in $(ALL_SETTINGS); do \
		$(MAKE) loopback SETTING=$$i; \
	done

clean:
	rm -f $(WORKDIR)/*.nhw

nhw: $(NHWIMAGES)
