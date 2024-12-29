import threading
from datetime import datetime
import os

os.environ["KIVY_NO_FILELOG"] = "1"
os.environ["KIVY_NO_CONSOLELOG"] = "0"
os.environ['KIVY_HOME'] = os.getcwd()

from kivy.config import Config
Config.set('input', 'mouse', 'mouse,disable_multitouch')
Config.set('graphics', 'width', 1000)
Config.set('graphics', 'height', 700)
Config.set('graphics', 'resizable', False)
Config.write()


from kivy.clock import Clock
from kivymd.app import MDApp
from kivymd.uix.boxlayout import MDBoxLayout
from kivymd.uix.menu import MDDropdownMenu
from kivymd.uix.button import MDFlatButton
from kivymd.uix.behaviors.toggle_behavior import MDToggleButton

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
from kivymd.uix.tooltip.tooltip import MDTooltip
from kivy.core.text import LabelBase

def resource_path(relative_path):
    """Get absolute path to resource, works for dev and for PyInstaller"""
    try:
        # PyInstaller creates a temp folder and stores path in _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")

    return os.path.join(base_path, relative_path)

LabelBase.register(name="Iconfont", fn_regular=resource_path(os.path.join("asset","icon.ttf")))

Builder.load_file(resource_path("ui.kv"))

# https://kivymd.readthedocs.io/en/1.1.1/behaviors/togglebutton/index.html
class MyToggleButton(MDFlatButton, MDToggleButton,MDTooltip):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.background_down = self.theme_cls.primary_color

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

        if self.ids.chk_volume.active:
            self.__current_processing_video.set_volume(float(self.ids.spinner_volume.value)/100)

        if self.ids.chk_flip_v.active:
            self.__current_processing_video.vflip()

        if self.ids.chk_flip_h.active:
            self.__current_processing_video.hflip()

        if self.ids.chk_rotate.active:
            angle = self.ids.layout_rotate.value
            if self.ids.btn_custom_rotate.state == "down":
                angle = self.ids.spinner_rotate.value
            print(angle)
            self.__current_processing_video.rotate(float(angle))

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
