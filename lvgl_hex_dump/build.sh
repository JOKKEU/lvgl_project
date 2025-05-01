#!/bin/bash

sudo gcc src/main.c src/file_op.c src/init.c src/lvgl.c -o file_dump -D DBG -I/home/jokkeu/Desktop/codes/lvgl_hex_dump -L/home/jokkeu/Desktop/codes/lvgl_hex_dump/lvgl/build/lib  -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu -lSDL2 -llvgl -lm





