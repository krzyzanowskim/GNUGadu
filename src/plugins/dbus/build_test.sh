# dbus-send example
# dbus-send --system /org/freedesktop/IM org.freedesktop.im.Signal.Ping string:'aaaaa'

gcc `pkg-config --libs dbus-glib-1 dbus-1 glib-2.0` `pkg-config --cflags dbus-glib-1 dbus-1 glib-2.0` test.c -o test