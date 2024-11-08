import os
import signal
import subprocess

base_path = os.path.dirname(os.path.abspath(__file__))


FFMPEG_PATH = os.path.join(
    base_path, "ffmpeg", "ffmpeg.exe" if os.name == "nt" else "ffmpeg"
)

FFPROBE_PATH = os.path.join(
    base_path, "ffmpeg", "ffprobe.exe" if os.name == "nt" else "ffprobe"
)


class Video:
    def __init__(self, input_file):
        self.clip = input_file
        self.filter_command = []
        self.extra_command = []
        self.__ffmpeg_command = [FFMPEG_PATH, "-i", input_file]
        self.__process = None

    def __build_command(self, output_file):
        self.__ffmpeg_command.extend(self.extra_command)
        if self.filter_command:
            self.__ffmpeg_command.append("-vf")
            self.__ffmpeg_command.append(",".join(self.filter_command))
        self.__ffmpeg_command.extend(["-progress", "pipe:2", output_file])

    def hflip(self):
        self.filter_command.append("hflip")

    def vflip(self):
        self.filter_command.append("vflip")

    def rotate(self, angle):
        pass

    def set_brightness(self, val):
        pass

    def set_contrast(self, val):
        pass

    def set_saturation(self, val):
        pass

    def scale(self, width, height):
        self.filter_command.append("scale=%d:%d" % (width, height))

    def set_aspect_ratio(self, ratio):
        self.filter_command.append("setdar=%s" % ratio)

    def set_volume(self, volume):
        sanitise_volume = (volume % 100) / 100
        self.extra_command.append("volume=%.6f" % sanitise_volume)

    def stereo_to_mono(self):
        self.extra_command.extend(["-ac", "1"])

    def __get_progress(self, line, video_duration_sec):
        if "out_time_ms" in line:
            try:
                time_processed = int(line.split("=")[1])
                seconds_processed = time_processed / 1_000_000
                return (seconds_processed / video_duration_sec) * 100
            except:
                return None
        return None

    def __get_video_duration(self, file_path):
        output = subprocess.run(
            [
                FFPROBE_PATH,
                "-v",
                "error",
                "-show_entries",
                "format=duration",
                "-of",
                "default=noprint_wrappers=1:nokey=1",
                file_path
            ],
            capture_output=True,
            text=True,
        ).stdout

        if output == None:
            return 0
        try:
            duration = float(output)
            return duration
        except:
            return 0

    def render(self, output_file, callback=None):
        video_duration = self.__get_video_duration(self.clip)
        if video_duration <= 0:
            return

        self.__build_command(output_file)
        self.__process = subprocess.Popen(
            self.__ffmpeg_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        print(self.__ffmpeg_command)
        if callback == None:
            callback = lambda _ : None
       
        while self.__process.poll() ==  None:
            line = self.__process.stderr.readline().strip()
            if line:
                progress = self.__get_progress(line, video_duration)
                if progress!=None:
                	callback(progress)
        callback(100)

    def get_ffmpeg_command(self):
        return self.__ffmpeg_command
