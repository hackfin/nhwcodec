NHWENC = ../encoder/nhwenc
NHWDEC = ../decoder/nhwdec

SETTING = h1

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

all: $(LOOPBACK_IMAGES)

clean:
	rm -f $(WORKDIR)/*.nhw

nhw: $(NHWIMAGES)
