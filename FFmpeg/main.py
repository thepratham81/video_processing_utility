#!/usr/bin/python3

from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.button import Button
from kivy.uix.label import Label
from kivy.uix.filechooser import FileChooser
import ffmpeg
import os

class MediaEditor(BoxLayout):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.orientation = 'vertical'
        
        # File chooser
        self.file_chooser = FileChooser()
        self.add_widget(self.file_chooser)

        # Edit buttons
        self.btn_layout = BoxLayout(size_hint_y=None, height='40dp')
        self.trim_btn = Button(text='Trim Video')
        self.trim_btn.bind(on_press=self.trim_video)
        self.btn_layout.add_widget(self.trim_btn)
        
        self.add_widget(self.btn_layout)

    def trim_video(self, instance):
        input_file = self.file_chooser.selection[0]
        output_file = os.path.join(os.path.dirname(input_file), 'output_trim.mp4')

        # FFmpeg command for trimming video (0-30 seconds)
        ffmpeg.input(input_file, ss=0, t=30).output(output_file).run()

        self.add_widget(Label(text=f"Trimmed video saved at: {output_file}"))


class MediaEditorApp(App):
    def build(self):
        return MediaEditor()

if __name__ == '__main__':
    MediaEditorApp().run()
