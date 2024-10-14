import os
import subprocess
from datetime import datetime
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.filechooser import FileChooserListView
from kivy.core.window import Window
from kivy.uix.popup import Popup


# Get the current time
current_time = datetime.now()
formatted_time = current_time.strftime("%Y-%m-%d_%H-%M-%S")

# Path to FFmpeg binaries
FFMPEG_PATH = os.path.join(os.getcwd(), 'ffmpeg', 'ffmpeg.exe')
print("FFMPEG_PATH is:",FFMPEG_PATH)

class MediaEditor(BoxLayout):
    def __init__(self, **kwargs):
        super(MediaEditor, self).__init__(**kwargs)
        self.orientation = 'vertical'

        # Label
        self.label = Label(text="Drag and Drop Media Files Here", size_hint=(1, 0.1))
        self.add_widget(self.label)

        # File chooser
        self.file_chooser = FileChooserListView(filters=['*.mp4', '*.mkv', '*.mp3', '*.wav'], size_hint=(1, 0.8))
        self.add_widget(self.file_chooser)

        # Buttons
        self.btn_edit = Button(text="Edit Media", size_hint=(1, 0.1))
        self.btn_edit.bind(on_press=self.edit_media)
        self.add_widget(self.btn_edit)

    def edit_media(self, instance):
        selected = self.file_chooser.selection
        #print("SELECTED :", selected)
        if selected:
            file_path = selected[0].strip() # Removes leading/trailing whitespace or newlines
            print("The selected file path is:", file_path)
            # Example: Trim first 10 seconds of the video/audio
            # Get file name and extension separately
            base, ext = os.path.splitext(file_path)
            output_file = f"{base}_Processed_{formatted_time}{ext}"
            print("Output file path is:", output_file)
            ffmpeg_command = f'"{FFMPEG_PATH}" -i "{file_path}" -t 10 -c copy "{output_file}"'
            try:
                subprocess.run(ffmpeg_command, check=True, shell=True)
                self.show_popup("Success", f"Edited file saved as {output_file}")
            except subprocess.CalledProcessError as e:
                self.show_popup("Error", "FFmpeg command failed")

    def show_popup(self, title, message):
        popup_layout = BoxLayout(orientation='vertical')
        popup_label = Label(text=message)
        popup_button = Button(text="OK", size_hint=(1, 0.2))
        popup_layout.add_widget(popup_label)
        popup_layout.add_widget(popup_button)

        popup = Popup(title=title, content=popup_layout, size_hint=(0.6, 0.4))
        popup_button.bind(on_press=popup.dismiss)
        popup.open()


class MediaEditorApp(App):
    def build(self):
        return MediaEditor()


if __name__ == '__main__':
    MediaEditorApp().run()
