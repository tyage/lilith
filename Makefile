include Makefile.common

SRCDIR := src

all: $(TARGET)

$(TARGET): build
	$(CP) $(SRCDIR)/$(TARGET) .

build:
	$(MAKE) -C $(SRCDIR) build

debug:
	$(MAKE) -C $(SRCDIR) debug
	$(CP) $(SRCDIR)/$(TARGET) .

.PHONY: clean clean_src
clean: clean_src
	$(RM) $(TARGET)

clean_src:
	$(MAKE) -C $(SRCDIR) clean
