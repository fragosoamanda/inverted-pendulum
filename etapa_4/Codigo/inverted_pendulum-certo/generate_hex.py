import os
Import("env")

def generate_hex(source, target, env):
    elf_path = str(target[0])
    hex_path = elf_path.replace(".elf", ".hex")

    # caminho do objcopy da toolchain do PlatformIO
    objcopy = env["OBJCOPY"] if "OBJCOPY" in env else "arm-none-eabi-objcopy"

    cmd = f"{objcopy} -O ihex {elf_path} {hex_path}"
    print(f"Gerando HEX: {cmd}")
    env.Execute(cmd)

# Adiciona a ação após o build do firmware.elf
env.AddPostAction("$BUILD_DIR/firmware.elf", generate_hex)
