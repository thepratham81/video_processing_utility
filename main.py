import threading
from datetime import datetime
import os
from kivy.clock import Clock
from kivymd.app import MDApp
from kivymd.uix.boxlayout import MDBoxLayout
from kivymd.uix.menu import MDDropdownMenu
from kivy.lang import Builder
from kivy.properties import (
    StringProperty,
    ObjectProperty,
    BooleanProperty,
    NumericProperty,
)
from kivy.core.window import Window
from functools import partial
from plyer import filechooser

import ffmpeg
from cspinner import CSpinner

kv = """
<CMDBoxLayout@MDBoxLayout>:
    spacing: dp(10)

<CheckItem>
    # adaptive_height: True
    adaptive_size:True
    active:chk.active

    MDCheckbox:
        id:chk
        adaptive_width: True
        active:root.active
    MDLabel:
        text: root.text
        adaptive_height: True
        adaptive_width:True
        padding_x: "12dp"

<FileItem>:
    MDLabel:
        text:root.text
    MDIconButton:
        icon: "delete"
        size_hint_x:None
        width:dp(48)
        on_state:if self.state == 'normal': root.delele_btn_pressed()

<FileList>:
    MDRecycleView:
        id:rv
        bar_width: 8
        scroll_type: ['bars']
        viewclass: 'FileItem'
        RecycleBoxLayout:
            padding: dp(10)
            spacing: dp(10)
            default_size: None, dp(48)
            default_size_hint: 1, None
            size_hint_y: None
            height: self.minimum_height
            orientation: "vertical"

<AppLayout>
    orientation:"vertical"
    spacing: dp(10)
    padding: dp(10)
    MDLabel:
        id:lbl_progress
        adaptive_height:True
        height:self.height * self.opacity
        opacity:0

    CMDBoxLayout:
        CMDBoxLayout:
            orientation:"vertical"
            MDLabel:
                text: "Drag and Drop File Here"
                adaptive_height:True
                size_hint_y:None
            FileList:
                id: file_list
                canvas.before:
                    Color:
                        rgba: .9, .9, .9, 1
                    Line:
                        width: 1
                        rectangle: self.x, self.y, self.width, self.height

            CMDBoxLayout:
                size_hint_y: None
                height: dp(48)
                MDRaisedButton:
                    text:"Add File(s)"
                    size_hint_x: 1
                    on_release:root.btn_add_file_clicked()
                MDRaisedButton:
                    text:"Remove All"
                    on_release : file_list.clear_file_list()
                    size_hint_x: 1
            
        GridLayout:
            cols: 1
            size_hint_y: None
            row_default_height: '48dp'
            row_force_default: True
            height: self.minimum_height
            pos_hint: {'top': 1, 'left': 1}  # Start from the top-left corner
            # CheckItem:
            #     id:chk_merge
            #     text: "Merge Video"
            CMDBoxLayout:
                CheckItem:
                    id:chk_rotate
                    text: "Rotate Video"
                CSpinner:
                    id:spinner_rotate
                    value:90
                    min:-360
                    max: 360
                    size_hint_x:None
                    width:dp(58)
                    disabled:not chk_rotate.active
                    opacity:int(chk_rotate.active)

            CheckItem:
                id:chk_flip_h
                text: "Flip horizontally"
            CheckItem:
                id:chk_flip_v
                text: "Flip vectically"
            CheckItem:
                id:chk_stereo_to_mono
                text: "Stereo to mono"


            CMDBoxLayout:
                CheckItem:
                    id:chk_scale_video
                    text: "Scale Video"
    
                CSpinner:
                    id:spinner_width
                    value:1920
                    min:float("-inf")
                    max:float("inf")
                    size_hint_x:None
                    width:dp(58)
                    disabled:not chk_scale_video.active
                    opacity:int(chk_scale_video.active)

                CSpinner:
                    id:spinner_height
                    value:1080
                    min:float("-inf")
                    max:float("inf")
                    size_hint_x:None
                    width:dp(58)
                    disabled:not chk_scale_video.active
                    opacity:int(chk_scale_video.active)

            CMDBoxLayout:
                CheckItem:
                    id:chk_brightness
                    text:"Change Brightness"
                CSpinner:
                    id:spinner_brightness
                    value:0
                    min:-100
                    max: 100
                    size_hint_x:None
                    width:dp(58)
                    disabled:not chk_brightness.active
                    opacity:int(chk_brightness.active)
            CMDBoxLayout:
                CheckItem:
                    id:chk_volume
                    text: "Change Volume"
                CSpinner:
                    id:spinner_volume
                    value:100
                    min:0
                    max: 100
                    size_hint_x:None
                    width:dp(58)
                    disabled:not chk_volume.active
                    opacity:int(chk_volume.active)

            CMDBoxLayout:
                CheckItem:
                    id:chk_aspect_ratio
                    text: "Change Aspect Ratio"
                MDDropDownItem:
                    id: drop_item_aspect_ratio
                    disabled: not chk_aspect_ratio.active
                    opacity:int(chk_aspect_ratio.active)
                    text: "16:9"
                    on_release: root.aspect_ratio_menu.open()

    CheckItem:
        id: chk_output_dir
        text: "Output Dir same as input dir"
        active: True

    CMDBoxLayout:
        adaptive_height: True
        height: dp(32)
        disabled:chk_output_dir.active
        MDTextFieldRect:
            id:txt_output_folder
            size_hint_x: 1
        MDRaisedButton:
            id: btn_browse
            text:"Browse"
            # size_hint_x: .2
            on_release:root.btn_select_folder()

    MDRaisedButton:
        id: btn_process
        size_hint_y: None
        height: dp(48)
        text:"Process Video"
        on_release: root.btn_process_video_clicked()
        size_hint_x: 1
        disabled: file_list.total_items == 0


"""

Builder.load_string(kv)


class CheckItem(MDBoxLayout):
    text = StringProperty()
    active = BooleanProperty(False)


class FileItem(MDBoxLayout):
    text = StringProperty()
    btn_delete_clicked = ObjectProperty()


class FileList(MDBoxLayout):

    total_items = NumericProperty(0)

    def __init__(self, **kwargs):
        Window.bind(on_drop_file=self.on_file_drop)
        Window.bind(on_drop_end=self.file_drop_end)
        super(FileList, self).__init__(**kwargs)

    def butt_release(self, index):
        for idx, i in enumerate(self.ids.rv.data):
            if i["text"] == index:
                self.ids.rv.data.pop(idx)
                break
        self.ids.rv.refresh_from_data()
        self.total_items = len(self.ids.rv.data)

    def on_file_drop(self, window, file_path, *args):
        video_file = file_path.decode("utf-8")

        file_extension = video_file.split(".")

        if file_extension[-1] not in ["mp4", "mkv"]:
            return

        self.ids.rv.data.append(
            {
                "text": video_file,
                "delele_btn_pressed": partial(self.butt_release, video_file),
            }
        )

    def file_drop_end(self, *args, **kwargs):
        self.ids.rv.refresh_from_data()
        self.total_items = len(self.ids.rv.data)

    def clear_file_list(self):
        self.ids.rv.data = []
        self.ids.rv.refresh_from_data()
        self.total_items = len(self.ids.rv.data)

    def get_all_file_name(self):
        return [i["text"] for i in self.ids.rv.data]

    def add_files(self, files):
        for file_path in files:
            self.ids.rv.data.append(
                {
                    "text": file_path,
                    "delele_btn_pressed": partial(self.butt_release, file_path),
                }
            )

        self.ids.rv.refresh_from_data()
        self.total_items = len(self.ids.rv.data)



class AppLayout(MDBoxLayout):
    def __init__(self, *args, **kwargs):
        super(AppLayout, self).__init__(*args, **kwargs)
        self.__stop_rendring = False
        self.__current_processing_video = None
        self.aspect_ratio_items = [
            {
                "viewClass": "MDLabel",
                "on_release": lambda x=str(ratio): self.set_item(
                    x, self.ids.drop_item_aspect_ratio, self.aspect_ratio_menu
                ),
                "text": str(ratio),
            }
            for ratio in [
                "16:9",
                "4:3",
                "1.85:1",
                "2.39:1",
                "3:2",
                "21:9",
                "5:4",
                "1:1",
                "18:9",
                "9:16",
                "32:9",
                "17:9",
                "2.76:1",
                "14:9",
                "16:10",
                "2:1",
                "4:1",
                "3:1",
                "1.66:1",
                "1.43:1",
            ]
        ]
        self.aspect_ratio_menu = MDDropdownMenu(
            caller=self.ids.drop_item_aspect_ratio,
            items=self.aspect_ratio_items,
            position="bottom",
            width_mult=4,
        )

        self.aspect_ratio_menu.bind()

    def progress_callback(self, progress,input_file_name , output_file_name):
        def hide_progress():
            self.ids.lbl_progress.opacity = 0

        self.ids.lbl_progress.text = "%.2f %s" % (progress,output_file_name)
        if progress == 100:
            self.ids.btn_process.text = "Process Video"
            self.ids.lbl_progress.text = ""
            Clock.schedule_once(lambda dt:hide_progress())
        else:
            self.ids.btn_process.text = "Stop Processing"

    def btn_add_file_clicked(self):
        files = filechooser.open_file(
            multiple=True,
            filters=[["Video (mp4, mkv)", "*.mp4", "*.mkv"], ["All Files", "*.*"]],
        )

        if files != None:
            self.ids.file_list.add_files(files)

    def btn_select_folder(self):
        folder = filechooser.choose_dir()
        if folder:
            self.ids.txt_output_folder.text = folder[0]

    def set_item(self, text_item, caller, menu):
        caller.set_item(text_item)
        menu.dismiss()

    def apply_filters(self):
        if self.__current_processing_video == None:
            print("No video to process")
            return

        if self.ids.chk_aspect_ratio.active:
            self.__current_processing_video.set_aspect_ratio(
                self.ids.drop_item_aspect_ratio.text.replace(":", "/")
            )
        if self.ids.chk_scale_video.active:
            self.__current_processing_video.scale(self.ids.spinner_width.value,
                                                  self.ids.spinner_height.value)
        if self.ids.chk_brightness.active:
            self.__current_processing_video.set_brightness(
                float(self.ids.spinner_brightness.value) / 100
            )

        if self.ids.chk_volume.active:
            self.__current_processing_video.set_volume(float(self.ids.spinner_volume.value)/100)

        if self.ids.chk_flip_h.active:
            self.__current_processing_video.vflip()

        if self.ids.chk_flip_h.active:
            self.__current_processing_video.hflip()

        if self.ids.chk_rotate.active:
            self.__current_processing_video.rotate(float(self.ids.spinner_rotate.value))

        if self.ids.chk_stereo_to_mono.active:
            self.__current_processing_video.stereo_to_mono()

    def __generate_file_name(self, file_path):
        # file_name.ext -> file_name_Processed_%Y-%m-%d_%H.%M.%S

        current_time = datetime.now()
        formatted_time = current_time.strftime("%Y-%m-%d_%H.%M.%S")

        _, file_name = os.path.split(file_path)
        file_name_without_ext, ext = os.path.splitext(file_name)

        return f"{file_name_without_ext}_Processed_{formatted_time}{ext}"

    def process_video(self):
        for video in self.ids.file_list.get_all_file_name():
            if self.__stop_rendring:
                break

            self.__current_processing_video = ffmpeg.Video(video)
            self.apply_filters()

            dir, file_name = os.path.split(video)
            file_name = self.__generate_file_name(file_name)
            output_path = None
            if self.ids.chk_output_dir.active:
                output_path = os.path.join(dir, file_name)
            else:
                if os.path.isdir(self.ids.txt_output_folder.text):
                    output_path = os.path.join(
                        self.ids.txt_output_folder.text, file_name
                    )

            if output_path:
                self.__current_processing_video.render(
                    output_path, callback=self.progress_callback
                )
        self.ids.btn_process.text = "Process Video"

    def btn_process_video_clicked(self):
        if self.ids.btn_process.text == "Stop All":
            self.ids.btn_process.text = "Process Video"
            if self.__current_processing_video != None:
                self.__stop_rendring = True
                self.__current_processing_video.stop_rendring()
        else:
            self.__stop_rendring = False
            self.ids.lbl_progress.opacity = 1
            t = threading.Thread(target=self.process_video)
            t.start()
            self.ids.btn_process.text = "Stop All"


class VpuApp(MDApp):
    def build(self):
        self.theme_cls.theme_style = "Dark"
        return AppLayout()


if __name__ == "__main__":
    app = VpuApp()
    app.run()
