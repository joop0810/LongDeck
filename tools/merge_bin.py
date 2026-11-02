# Erzeugt nach dem Build eine einzelne Firmware-Datei (bootloader + partitions + app)
# für den Web-Installer: firmware/longdeck.bin (Flash-Adresse 0x0)
Import("env")
import os


def merge_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    out_dir = os.path.join(project_dir, "firmware")
    os.makedirs(out_dir, exist_ok=True)
    out_file = os.path.join(out_dir, "longdeck.bin")

    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    boot_app0 = os.path.join(framework_dir, "tools", "partitions", "boot_app0.bin")

    images = [
        ("0x0", os.path.join(build_dir, "bootloader.bin")),
        ("0x8000", os.path.join(build_dir, "partitions.bin")),
        ("0xe000", boot_app0),
        ("0x10000", os.path.join(build_dir, "firmware.bin")),
    ]
    args = []
    for addr, path in images:
        args += [addr, '"%s"' % path]

    cmd = " ".join([
        '"$PYTHONEXE"', '"%s"' % os.path.join(env.PioPlatform().get_package_dir("tool-esptoolpy"), "esptool.py"),
        "--chip esp32s3 merge_bin",
        "-o", '"%s"' % out_file,
        "--flash_mode keep --flash_freq keep --flash_size 16MB",
    ] + args)
    env.Execute(cmd)
    print(">>> Web-Installer-Image: %s" % out_file)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)
