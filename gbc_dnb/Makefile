# Makefile — gbc_dnb (DnB III-style sampled-break player for Game Boy)
GBDK_HOME := ../gbc/gbdk/
LCC := $(GBDK_HOME)bin/lcc
LCCFLAGS := -Wl-yt1 -Wl-yo16

PROJECT := dnb3
SRCDIR := src
RESDIR := res
OBJDIR := obj

CSOURCES := $(notdir $(wildcard $(SRCDIR)/*.c)) $(notdir $(wildcard $(RESDIR)/*.c))
OBJS := $(CSOURCES:%.c=$(OBJDIR)/%.o)

all: prepare $(OBJDIR)/$(PROJECT).gb

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(RESDIR)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

$(OBJDIR)/$(PROJECT).gb: $(OBJS)
	$(LCC) $(LCCFLAGS) -o $@ $(OBJS)
	@cp $@ $(PROJECT).gb
	@ls -la $(PROJECT).gb

prepare:
	@mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) *.gb

.PHONY: all clean prepare
