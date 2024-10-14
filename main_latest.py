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
formatted_time = current_time.strftime("%Y-%m-%d_%H.%M.%S")

# Path to FFmpeg binaries
FFMPEG_PATH = os.path.join(os.getcwd(), 'ffmpeg', 'ffmpeg.exe')
print("FFMPEG_PATH is:", FFMPEG_PATH)

class MediaEditor(BoxLayout):
    def __init__(self, **kwargs):
        super(MediaEditor, self).__init__(**kwargs)
        self.orientation = 'vertical'

        # Label
        self.label = Label(text="Drag and Drop Media Files Here", size_hint=(1, 0.1))
        self.add_widget(self.label)

        # File chooser
        self.file_chooser = FileChooserListView(filters=['*.mp4', '*.mkv', '*.mp3', '*.wav'], size_hint=(1, 0.6))
        self.add_widget(self.file_chooser)

        # Buttons layout for media editing options
        self.btn_layout = BoxLayout(size_hint=(1, 0.2))
        
        # Rotate 90° Clockwise
        self.btn_rotate_90 = Button(text="Rotate 90°")
        self.btn_rotate_90.bind(on_press=lambda x: self.edit_media("rotate_90"))
        self.btn_layout.add_widget(self.btn_rotate_90)

        # Rotate 180° Clockwise
        self.btn_rotate_180 = Button(text="Rotate 180°")
        self.btn_rotate_180.bind(on_press=lambda x: self.edit_media("rotate_180"))
        self.btn_layout.add_widget(self.btn_rotate_180)

        # Flip Horizontally
        self.btn_flip_h = Button(text="Flip Horizontally")
        self.btn_flip_h.bind(on_press=lambda x: self.edit_media("flip_h"))
        self.btn_layout.add_widget(self.btn_flip_h)

        # Flip Vertically
        self.btn_flip_v = Button(text="Flip Vertically")
        self.btn_flip_v.bind(on_press=lambda x: self.edit_media("flip_v"))
        self.btn_layout.add_widget(self.btn_flip_v)

        # Change Aspect Ratio
        self.btn_aspect_ratio = Button(text="Change Aspect Ratio")
        self.btn_aspect_ratio.bind(on_press=lambda x: self.edit_media("aspect_ratio"))
        self.btn_layout.add_widget(self.btn_aspect_ratio)

        # Convert Stereo to Mono
        self.btn_stereo_to_mono = Button(text="Stereo to Mono")
        self.btn_stereo_to_mono.bind(on_press=lambda x: self.edit_media("stereo_to_mono"))
        self.btn_layout.add_widget(self.btn_stereo_to_mono)

        self.add_widget(self.btn_layout)

    def edit_media(self, action):
        selected = self.file_chooser.selection
        if selected:
            file_path = selected[0].strip()  # Removes leading/trailing whitespace or newlines
            base, ext = os.path.splitext(file_path)
            output_file = f"{base}_Processed_{formatted_time}{ext}"
            print(f"Processing {action} on file {file_path}")

            # Build the appropriate FFmpeg command based on the action
            ffmpeg_command = self.build_ffmpeg_command(file_path, action, output_file)
            
            if ffmpeg_command:
                try:
                    subprocess.run(ffmpeg_command, check=True, shell=True)
                    self.show_popup("Success", f"Edited file saved as {output_file}")
                except subprocess.CalledProcessError as e:
                    self.show_popup("Error", f"FFmpeg command failed: {e}")
            else:
                self.show_popup("Error", "Invalid action")

    def build_ffmpeg_command(self, file_path, action, output_file):
        """Build FFmpeg command based on the requested action."""
        if action == "rotate_90":
            return f'"{FFMPEG_PATH}" -i "{file_path}" -vf "transpose=1" "{output_file}"'
        elif action == "rotate_180":
            return f'"{FFMPEG_PATH}" -i "{file_path}" -vf "transpose=2" "{output_file}"'
        elif action == "flip_h":
            return f'"{FFMPEG_PATH}" -i "{file_path}" -vf "hflip" "{output_file}"'
        elif action == "flip_v":
            return f'"{FFMPEG_PATH}" -i "{file_path}" -vf "vflip" "{output_file}"'
        elif action == "aspect_ratio":
            # Change aspect ratio to 16:9 for example
            return f'"{FFMPEG_PATH}" -i "{file_path}" -vf "scale=1280:720" "{output_file}"'
        elif action == "stereo_to_mono":
            # Convert stereo audio to mono
            return f'"{FFMPEG_PATH}" -i "{file_path}" -ac 1 "{output_file}"'
        return None

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