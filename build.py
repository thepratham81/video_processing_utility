import urllib.request
import platform
import zipfile
import os

import shutil
import pathlib
import tarfile
import subprocess
import sys

os_name = platform.system()
build_folder = "VPU_EXE"
ffmpeg_folder = os.path.join(build_folder,"ffmpeg")
DEPENDENCIES = ["kivy", "kivymd", "plyer", "pyinstaller"]


def install_dependencies():
    for i in DEPENDENCIES:
        print("Installing", i)
        subprocess.check_call([sys.executable, "-m", "pip", "install", i])


def build_executable():
    pyinstaller_path = os.path.join(
        os.path.dirname(sys.executable),
        "pyinstaller" if os_name != "Windows" else "pyinstaller.exe",
    )
    executable_name = "main" + ("" if os_name != "Windows" else ".exe")
    executable_path = os.path.join("dist",executable_name)
    if os.path.exists(executable_path):
        os.remove(executable_path)

    print(f"Building executable..")
    print(f"{pyinstaller_path} main.spec")
    subprocess.check_call([pyinstaller_path,"main.spec"])
    os.makedirs(build_folder,exist_ok = True)
    shutil.copy(executable_path,build_folder)








def extract_file_from_tarxz(tarxz_path, file_to_extract, output_dir):
    out = pathlib.Path(output_dir)

    # Ensure the destination directory exists
    os.makedirs(out, exist_ok=True)

    with tarfile.open(tarxz_path, "r:xz") as archive:
        # Check if the file exists in the archive
        if file_to_extract in archive.getnames():
            destpath = out.joinpath(
                file_to_extract.split("/")[-1]
            )  # Get the filename from the path
            with open(destpath, "wb") as dest:
                file = archive.extractfile(file_to_extract)
                dest.write(file.read())


def extract_file_from_zip(zip_path, file_to_extract, output_dir):
    archive = zipfile.ZipFile(zip_path)
    out = pathlib.Path(output_dir)

    # Ensure the destination directory exists
    os.makedirs(out, exist_ok=True)

    # Check if the file exists in the archive
    if file_to_extract in archive.namelist():
        destpath = out.joinpath(
            file_to_extract.split("/")[-1]
        )  # Get the filename from the path
        with archive.open(file_to_extract) as source, open(destpath, "wb") as dest:
            shutil.copyfileobj(source, dest)


def download_file(url, out_file_name):
    def progress(block_num, block_size, total_size):
        downloaded = block_num * block_size
        percent = min(100, downloaded * 100 / total_size)
        print(f"\rDownloading: {percent:.2f}%", end="")

    try:
        if not os.path.exists(out_file_name):
            urllib.request.urlretrieve(url, out_file_name, reporthook=progress)
            print("\nDownload complete.")
    except Exception as e:
        print(f"\nDownload failed: {e}")


install_dependencies()
if not os.path.exists(ffmpeg_folder):
    os.makedirs(ffmpeg_folder)

if os_name == "Windows":
    ffmpeg_url = "https://github.com/GyanD/codexffmpeg/releases/download/7.1/ffmpeg-7.1-essentials_build.zip"

    download_file(ffmpeg_url, "ffmpeg.zip")

    if not os.path.exists(os.path.join(ffmpeg_folder, "ffmpeg.exe")):

        print("Extracting ffmpeg.exe..")
        extract_file_from_zip(
            "ffmpeg.zip", "ffmpeg-7.1-essentials_build/bin/ffmpeg.exe",ffmpeg_folder
        )

    if not os.path.exists(os.path.join(ffmpeg_folder, "ffprobe.exe")):
        print("Extracting ffprobe.exe..")
        extract_file_from_zip(
            "ffmpeg.zip", "ffmpeg-7.1-essentials_build/bin/ffprobe.exe", ffmpeg_folder        )

elif os_name == "Linux":
    ffmpeg_url = (
        "https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-amd64-static.tar.xz"
    )
    download_file(ffmpeg_url, "ffmpeg.tar.xz")

    if not os.path.exists(os.path.join(ffmpeg_folder, "ffmpeg")):
        print("Extracting ffmpeg..")
        extract_file_from_tarxz(
            "ffmpeg.tar.xz", "ffmpeg-7.0.2-amd64-static/ffmpeg", ffmpeg_folder
        )

    if not os.path.exists(os.path.join(ffmpeg_folder, "ffprobe")):
        print("Extracting ffprobe ..")
        extract_file_from_tarxz(
            "ffmpeg.tar.xz", "ffmpeg-7.0.2-amd64-static/ffprobe", ffmpeg_folder
        )
build_executable()

