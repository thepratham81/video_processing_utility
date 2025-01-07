import os
import math
import signal
import subprocess
import json
from io import BytesIO
import tempfile

base_path = os.path.dirname(os.path.abspath(__file__))


FFMPEG_PATH = os.path.join(
    base_path, "ffmpeg", "ffmpeg.exe" if os.name == "nt" else "ffmpeg"
)

FFPROBE_PATH = os.path.join(
    base_path, "ffmpeg", "ffprobe.exe" if os.name == "nt" else "ffprobe"
)

def get_thumbnail(file_path, index,height = -1):
    # Create a temporary file to store the thumbnail
    with tempfile.NamedTemporaryFile(delete=True, suffix=".png") as temp_file:
        temp_file_path = temp_file.name

    command = [
        FFMPEG_PATH, "-i", file_path, "-map", f"0:{index}", "-vf", f"scale=-1:{height}", "-c:v", "png", "-f", "image2pipe", "-"
    ]
    
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        # Write the image data to the temporary file
        with open(temp_file_path, 'wb') as temp_file:
            temp_file.write(result.stdout)

        return temp_file_path

    except subprocess.CalledProcessError as e:
        print(f"Error occurred: {e.stderr.decode()}")
        return None
    except Exception as e:
        print(f"Unexpected error: {str(e)}")
        return None


def generate_thumbnail(file_path, time, height=-1):
    with tempfile.NamedTemporaryFile(delete=True, suffix=".png") as temp_file:
        temp_file_path = temp_file.name

    command = [
        FFMPEG_PATH, "-i", file_path, "-ss", str(time), "-vframes", "1", "-vf", f"scale=-1:{height}", "-c:v", "png", temp_file_path
    ]
    
    try:
        subprocess.run(command, check=True)
        return temp_file_path
    except subprocess.CalledProcessError as e:
        print(f"Error occurred: {e.stderr.decode()}")
        return None
    except Exception as e:
        print(f"Unexpected error: {str(e)}")
        return None

def get_video_info( file_path):
    command = [
        FFPROBE_PATH,
        "-v",
        "error",
        "-print_format",
        "json",
        "-show_format",
        "-show_streams",
        file_path,
    ]
    output = subprocess.run(command, capture_output=True, text=True).stdout
    if output:
        return json.loads(output)
    return None

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
        angle = float(angle)
        if angle == 90:
            self.filter_command.append("transpose=1")
        elif angle == 180:
            self.filter_command.append("transpose=1,transpose=1")
        elif angle == 270:
            self.filter_command.append("transpose=1,transpose=1,transpose=1")
        else:
            rad = math.radians(angle)
            self.filter_command.append("rotate=%.6f"%rad)




    def set_brightness(self, val):
        self.filter_command.append("eq=brightness=%.6f"%float(val))

    def set_contrast(self, val):
        self.filter_command.append("eq=contrast=%.6f"%float(val))

    def set_saturation(self, val):
        self.filter_command.append("q=saturation=%.6f"%float(val))

    def scale(self, width, height):
        self.filter_command.append("scale=%d:%d" % (width, height))

    def set_aspect_ratio(self, ratio):
        self.filter_command.append("setdar=%s" % ratio)

    def set_volume(self, volume):
        self.extra_command.append("-af")
        self.extra_command.append("volume=%.6f" % volume)

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
            callback = lambda _,__,___ : None
       
        while self.__process.poll() ==  None:
            line = self.__process.stderr.readline().strip()
            if line:
                progress = self.__get_progress(line, video_duration)
                if progress!=None:
                	callback(progress,self.clip,output_file)
        callback(100,self.clip,output_file)

    def get_ffmpeg_command(self):
        return self.__ffmpeg_command

    def stop_rendring(self):
        if self.__process:
            self.__process.kill()

