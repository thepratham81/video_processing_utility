# Work in progress

# Project Structure

```
.
├── compile_flags.txt
├── include
│   ├── cstring.h
│   ├── file_util.h
│   ├── list.h
│   ├── nuklear.h
│   ├── nuklear_sdl_renderer.h
│   ├── password_input.h
│   ├── stb_image.h
│   ├── stb_to_sdl.h
│   ├── subprocess.h
│   ├── ui
│   │   └── main_ui.h
│   ├── vector.h
│   └── video.h
├── meson.build
├── README.md
├── src
│   ├── ffmpeg_wrap.c
│   ├── list.c
│   ├── main.c
│   ├── resources.c
│   ├── ui
│   │   └── main_ui.c
│   └── utils
│       ├── file_dialog.c
│       └── file_util.c
├── subprojects
│   └── sdl2.wrap
└── x86_64-w64-mingw32.txt
```

# How to build it 

```
git clone git@github.com:thepratham81/video_processing_utility.git
cd video_processing_utility
meson setup --buildtype=release build
meson compile -C build
```
after building you can run :-

```
./build/vpu
```

# Library used

[subprocess.h](https://github.com/sheredom/subprocess.h) for creating subprocess.
[Nuklear](https://github.com/Immediate-Mode-UI/Nuklear) for creating GUI.
