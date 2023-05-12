build:
	$(CC) mouse_switcher.c -o mouse_switcher -O2 `pkg-config --cflags --libs libevdev`

install:
	install mouse_switcher /usr/bin

uninstall:
	rm /usr/bin/mouse_switcher

clean:
	rm mouse_switcher
