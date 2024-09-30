from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.button import Button
from kivy.uix.label import Label
from kivy.uix.slider import Slider
from kivy.uix.filechooser import FileChooserIconView
from kivy.uix.popup import Popup
import ffmpeg
import os

class MediaEditor(BoxLayout):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.orientation = 'vertical'

        # Label to show the selected file
        self.file_label = Label(text='Drag and Drop a Media File', size_hint_y=0.1)
        self.add_widget(self.file_label)

        # File chooser area for drag-and-drop
        self.file_chooser = FileChooserIconView(filters=["*.mp4", "*.avi", "*.mkv"], multiselect=False)
        self.file_chooser.bind(on_selection=self.on_file_select)
        self.add_widget(self.file_chooser)

        # Sliders for trimming
        self.slider_layout = BoxLayout(orientation='horizontal', size_hint_y=0.2)
        
        self.start_slider = Slider(min=0, max=100, value=0)
        self.end_slider = Slider(min=0, max=100, value=100)
        self.slider_layout.add_widget(Label(text='Start'))
        self.slider_layout.add_widget(self.start_slider)
        self.slider_layout.add_widget(Label(text='End'))
        self.slider_layout.add_widget(self.end_slider)
        
        self.add_widget(self.slider_layout)

        # Button to trim the video
        self.trim_btn = Button(text='Trim Video', size_hint_y=0.1)
        self.trim_btn.bind(on_press=self.trim_video)
        self.add_widget(self.trim_btn)

        # Variables to hold the selected file and the trimmed output path
        self.input_file = None
        self.output_file = None

    def on_file_select(self, filechooser, selected_files):
        """
        Handle the file selection or drag-and-drop event
        """
        if selected_files:  # Check if any files were selected
            self.input_file = selected_files[0]  # Get the first selected file
            self.file_label.text = f'Selected: {self.input_file}'
            
            # Dynamically update the slider range based on the video duration
            self.update_slider_range(self.input_file)
        else:
            self.file_label.text = 'No file selected!'

    def update_slider_range(self, file_path):
        """
        Update slider max values based on the video duration using FFmpeg probe
        """
        try:
            probe = ffmpeg.probe(file_path)
            duration = float(probe['format']['duration'])

            # Update slider max values based on duration
            self.start_slider.max = duration
            self.end_slider.max = duration
        except Exception as e:
            popup = Popup(title='Error', content=Label(text=f'Error reading file: {str(e)}'), size_hint=(0.5, 0.5))
            popup.open()

    def trim_video(self, instance):
        """
        Trim the selected video based on the slider values
        """
        if not self.input_file:
            popup = Popup(title='Error', content=Label(text="No file selected!"), size_hint=(0.5, 0.5))
            popup.open()
            return

        # Get the start and end times from the sliders
        start_time = self.start_slider.value
        end_time = self.end_slider.value

        if start_time >= end_time:
            popup = Popup(title='Error', content=Label(text="Invalid trim range!"), size_hint=(0.5, 0.5))
            popup.open()
            return

        # Set output file path
        self.output_file = os.path.join(os.path.dirname(self.input_file), 'output_trim.mp4')

        try:
            # Run FFmpeg to trim the video
            ffmpeg.input(self.input_file, ss=start_time, to=end_time).output(self.output_file).run()

            popup = Popup(title='Success', content=Label(text=f'Trimmed video saved at: {self.output_file}'), size_hint=(0.5, 0.5))
            popup.open()
        except Exception as e:
            popup = Popup(title='Error', content=Label(text=f'Error during trimming: {str(e)}'), size_hint=(0.5, 0.5))
            popup.open()


class MediaEditorApp(App):
    def build(self):
        return MediaEditor()

if __name__ == '__main__':
    MediaEditorApp().run()
