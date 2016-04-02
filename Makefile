netns-switch: netns-switch.c
	$(CC) $(CFLAGS) -o $@ $<

install: netns-switch
	install --group=root --owner=root --mode=4755 $< $(BINDIR)

clean:
	rm -f netns-switch

all: netns-switch
.PHONY: clean
