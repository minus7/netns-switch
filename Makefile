netns-switch: netns-switch.c
	$(CC) $(CFLAGS) -o $@ $<

all: netns-switch

install: netns-switch
	install --group=root --owner=root --mode=4755 $< $(BINDIR)
