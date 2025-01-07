import threading
from datetime import datetime
import os

# os.environ["KIVY_NO_FILELOG"] = "1"
# os.environ["KIVY_NO_CONSOLELOG"] = "0"
os.environ['KIVY_HOME'] = os.getcwd()

from kivy.config import Config
Config.set('input', 'mouse', 'mouse,disable_multitouch')
Config.set('graphics', 'width', 1000)
Config.set('graphics', 'height', 800)
Config.set('graphics', 'resizable', False)
Config.write()


from kivy.clock import Clock
from kivymd.app import MDApp
from kivymd.uix.boxlayout import MDBoxLayout
from kivymd.uix.menu import MDDropdownMenu
from kivymd.uix.button import MDFlatButton
from kivy.uix.widget import Widget
from kivy.uix.label import Label
from kivymd.uix.behaviors.toggle_behavior import MDToggleButton
from kivy.graphics import Color, Rectangle,Line

from kivy.uix.image import Image
from PIL import Image as PILImage, ImageOps
from io import BytesIO
from kivy.core.image import Image as CoreImage
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

class AspectRatioWidget(Widget):
    aspect_width = NumericProperty(1)
    aspect_height = NumericProperty(1)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.bind(size=self.update_canvas)
        self.bind(pos=self.update_canvas)
        self.bind(aspect_width=self.update_canvas)
        self.bind(aspect_height=self.update_canvas)


    def update_canvas(self, *args):
        self.canvas.clear()

        border_thickness = 1

        width = self.width  - 2*border_thickness
        height = self.height  - 2*border_thickness
 

        box_width = width
        box_height = width  * (self.aspect_height/self.aspect_width)
        if box_height > height :
            box_height = height 
            box_width = height  * (self.aspect_width/self.aspect_height)

        x = (width  - box_width) / 2
        y = (height  - box_height) / 2
        with self.canvas:
            # border color
            Color(1,1,1,1)
            Line(rectangle=(self.pos[0], self.pos[1],width , height ), width=border_thickness)

            # background color
            Color(0,0,0,1)
            Rectangle(pos=self.pos, size=(width , height ))

            # inner rectengle color
            Color(1,1,1,1)
            Rectangle(pos=(self.pos[0]+x, self.pos[1]+y), size=(box_width, box_height))



class RotatedImage(MDBoxLayout):
    angle = NumericProperty(0)
    flip_h = BooleanProperty(False)
    flip_v = BooleanProperty(False)
    source = StringProperty()
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.bind(flip_h=self.on_flip_h, flip_v=self.on_flip_v)
        self.image = Image(source=self.source)
        self.label = Label(text='No image available', font_size=32, color=(1, 1, 1, 1))
        
        if self.source:
            self.add_widget(self.image)
        else:
            self.add_widget(self.label)

    def on_source(self,instance,value):
        if self.source:
            self.clear_widgets()
            self.image.source = self.source
            self.add_widget(self.image)
        else:
            self.clear_widgets()
            self.add_widget(self.label)

    def on_angle(self, instance, value):
        self.update_image()

    def on_flip_h(self, instance, value):
        self.update_image()

    def on_flip_v(self, instance, value):
        self.update_image()

    def update_image(self):
        if not self.image.source:
            return
        pil_image = PILImage.open(self.source).convert("RGBA")
        
        if self.flip_h:
            pil_image = ImageOps.mirror(pil_image)
        if self.flip_v:
            pil_image = ImageOps.flip(pil_image)

        pil_image = pil_image.rotate(-self.angle, expand=True, resample=PILImage.BICUBIC)
        
        # Convert PIL image to texture
        self.texture = self._pil_to_texture(pil_image)

    def _pil_to_texture(self, pil_image):
        # Convert the PIL image to PNG bytes
        byte_io = BytesIO()
        pil_image.save(byte_io, format='PNG')
        byte_io.seek(0)
        
        # Create a texture from the PNG bytes
        core_image = CoreImage(byte_io, ext="png")
        texture = core_image.texture
        return texture

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

Builder.load_string('''
#:import utils kivy.utils
<CMDLabel@MDLabel>:
    adaptive_height: True
    adaptive_width:True

<VideoInfoWidget>:
    orientation:"vertical"
    RotatedImage:
        id: video_thumbnail
        flip_h:root.flip_h
        flip_v:root.flip_v
        angle: root.angle
    MDBoxLayout:
        orientation:"vertical"
        adaptive_size:True
        opacity: int(len(root.video) > 0)
        CMDLabel:
            text:"File Path: "+root.video
        CMDLabel:
            text: "File Size: "+utils.format_bytes_to_human(root.file_size)
        CMDLabel:
            text: "Bit Rate: "+utils.format_bytes_to_human(root.bitrate)
        CMDLabel:
            text:"Width: "+str(root.coded_width)
        CMDLabel:
            text: "Height: "+str(root.coded_height)


''')
class VideoInfoWidget(MDBoxLayout):
    video = StringProperty()
    file_size = NumericProperty()
    bitrate = NumericProperty()
    coded_width = NumericProperty()
    coded_height = NumericProperty()
    angle = NumericProperty(0)
    flip_h = BooleanProperty(False)
    flip_v = BooleanProperty(False)

    aspect_ratio_w = StringProperty()

    def __init__(self,*args,**kwargs):
        super(VideoInfoWidget, self).__init__(*args,**kwargs)
        Window.bind(on_drop_file=self.on_file_drop)
        Window.bind(on_drop_end=self.file_drop_end)
        self.bind(flip_h=self.on_flip_h, flip_v=self.on_flip_v)

    def on_angle(self,instance,value):
        if len(self.video) != 0:
            self.angle = value

    def on_flip_h(self,instance,value):
        if len(self.video) != 0:
            self.flip_h = True

    def on_flip_v(self,instance,value):
        if len(self.video) != 0:
            selfflip_v = True

    def __helper(self, video):
        def change_thumbnail_image(res):
            self.ids.video_thumbnail.source = res

        output = ffmpeg.get_video_info(video)
        if output is None:
            return

        format = output['format']
        self.file_size = format['size']
        self.bitrate = format['bit_rate']

        thumbnail_found = False

        for stream in output['streams']:
            codec_name = stream['codec_type']
            if codec_name == 'video' and stream['codec_name']!='png':
                self.coded_width = stream['width']
                self.coded_height = stream['height']

            if not thumbnail_found and stream['codec_name'] == "png":
                res = ffmpeg.get_thumbnail(video, stream['index'])
                thumbnail_found = True
                Clock.schedule_once(lambda dt:change_thumbnail_image(res))

        if not thumbnail_found:
            # TODO generate thumbnail from video
            Clock.schedule_once(lambda dt:change_thumbnail_image(""))

                
    def on_file_drop(self, window, file_path, *args):
        video_file = file_path.decode("utf-8")
        file_extension = video_file.split(".")
        if file_extension[-1] not in ["mp4", "mkv"]:
            self.video = ""
            return
        self.video  = video_file


    def file_drop_end(self, *args, **kwargs):
        pass

    def on_video(self,instance,video):
        if self.video:
            thread = threading.Thread(target=self.__helper,args=(video,))
            thread.start()
        else:
            self.ids.video_thumbnail.source = ""



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
            self.ids.btn_process.text = "Render"
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
        w,h = map(float, text_item.split(":"))
        self.ids.aspect_feedback.aspect_width = w
        self.ids.aspect_feedback.aspect_height= h
        menu.dismiss()

    def abc(self,value):
        aw = self.ids.video_info.coded_width
        ah = self.ids.video_info.coded_height 
        w = self.ids.spinner_width.value
        print(aw,ah,w,( w * ah )/aw)
        self.ids.spinner_height.value = (( w * ah )/aw)
        print(self.ids.spinner_height.value)

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
        self.ids.btn_process.text = "Render"

    def btn_process_video_clicked(self):
        if self.ids.btn_process.text == "Stop Rendring":
            self.ids.btn_process.text = "Render"
            if self.__current_processing_video != None:
                self.__stop_rendring = True
                self.__current_processing_video.stop_rendring()
        else:
            self.__stop_rendring = False
            self.ids.lbl_progress.opacity = 1
            t = threading.Thread(target=self.process_video)
            t.start()
            self.ids.btn_process.text = "Stop Rendring"


class VpuApp(MDApp):
    def build(self):
        self.theme_cls.theme_style = "Dark"
        return AppLayout()


if __name__ == "__main__":
    app = VpuApp()
    app.run()
