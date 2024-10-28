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
# Install

You don't need  to install it. It is portable. just extract it and double click vpu(on linux ) or vpu.exe(on windows).<br>
[windows x86_64](https://github.com/thepratham81/video_processing_utility/releases/download/0.2/vpu_win_x86_64.zip)<br>
[linux x86_64](https://github.com/thepratham81/video_processing_utility/releases/download/0.2/vpu_linux_x86_64.tar.xz)<br>

For mac , compile your self

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

[subprocess.h](https://github.com/sheredom/subprocess.h) for creating subprocess.<br>
[Nuklear](https://github.com/Immediate-Mode-UI/Nuklear) for creating GUI.<br>
[stb](https://github.com/nothings/stb) single header library.<br>
[ffmpeg](https://www.ffmpeg.org) for video processing.<br>

