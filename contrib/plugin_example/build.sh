#! /bin/sh
gcc my_plugin.c -o my_plugin.so -shared `pkg-config gg2_core --libs --cflags`