#set text(
  font: "Anonymous Pro"
)

#show link: underline

= Introduction


= Features which donot require feedback.
- Rotate (90°,180°,270°,custom)
- Flip horizontally 
- Flip Vertically 
- Add thumbnail
- Sterio to mono

= Features which require feedback.
- Crop
- Trim
- Change brightness
- Resizing
  - with ascpect ratio
  - without ascpect ratio
  - It will contain dropdown with common aspect ratio
- Adjust volume

= Advance Features
- Repair(container failure repair)
  - user need to provide both currpted file and a healthy file 
- Transcoding
- Passthough
- Add/Remove subtiltle

= Trim Feature
It will contain a timeline, where user can zoom in and out in
time line, and can also seek frame by frame.

= Technology used

*Programming langugage*: C \
*GUI Framework*: #link("https://github.com/cimgui/cimgui")[cimgui]
(#link("https://github.com/ocornut/imgui")[imgui] binding for c) \
*build system*:  #link("https://github.com/tsoding/nob.h")[nob.h]\
*Other*: #link("https://ffmpeg.org/")[ ffmpeg ]


= FAQ

== why C Programming language?
C is minimal, fast, and, more importantly,
I am comfortable with it.

== Why ImGui as the GUI framework?
Because I am comfortable with it, unlike GTK, WxWidgets, or Qt.
