Import("env")
from shutil import copyfile

def move_bin(*args, **kwargs):
    print("Copying bin output to project directory...")
    target = str(kwargs['target'][0])
    if target == ".pio/build/esp32/firmware.bin":
        copyfile(target, 'esp32_air_quality.bin')
    elif target == ".pio/build/esp32c3/firmware.bin":
        copyfile(target, 'esp32c3_air_quality.bin')
    print("Done.")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", move_bin)   #post action for .bin