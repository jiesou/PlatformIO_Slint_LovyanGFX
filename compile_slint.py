#!/usr/bin/env python3
import os
import sys
import shutil
import tarfile
import requests
import platform
import subprocess
from pathlib import Path

# Hardcoded Slint version for now
if os.environ.get("SLINT_VERSION"):
    SLINT_VERSION = os.environ.get("SLINT_VERSION")
else:
    SLINT_VERSION = "1.14.1"

# See: https://github.com/slint-ui/slint/blob/e41a460b135175a570e39c95985ded246e368bfb/api/cpp/esp-idf/slint/cmake/FindSlint.cmake
def get_target_arch(env):
    board_config = env.BoardConfig()
    mcu = board_config.get("build.mcu", "esp32")
    print(f"[Slint_LovyanGFX] Detected MCU: {mcu}")
    
    if mcu == "esp32s3":
        return "xtensa-esp32s3-none-elf"
    elif mcu == "esp32":
        return "xtensa-esp32-none-elf"
    elif mcu == "esp32s2":
        return "xtensa-esp32s2-none-elf"
    elif mcu in ["esp32c6", "esp32c5", "esp32h2"]:
        return "riscv32imac-esp-espidf"
    elif mcu == "esp32p4":
        return "riscv32imafc-esp-espidf"
    
    print(f"Warning: Unknown MCU '{mcu}', defaulting to xtensa-esp32-none-elf")
    return "xtensa-esp32-none-elf"

def get_host_platform():
    system = platform.system()
    machine = platform.machine()
    
    if system == "Linux":
        if machine == "x86_64":
            return "Linux-x86_64"
        elif machine == "aarch64":
            return "Linux-aarch64"
    elif system == "Windows":
        if machine == "AMD64":
            return "Windows-AMD64"
        elif machine == "ARM64":
            return "Windows-ARM64"
    elif system == "Darwin":
        return "Darwin-arm64"
    
    return f"{system}-{machine}"

def download_file(url, dest_path):
    print(f"Downloading {url}...")
    response = requests.get(url, stream=True)
    if response.status_code != 200:
        sys.stderr.write(f"Failed to download: {response.status_code}\n")
        sys.exit(1)
        
    total_length = response.headers.get('content-length')

    with open(dest_path, "wb") as f:
        if total_length is None:
            for chunk in response.iter_content(chunk_size=8192):
                f.write(chunk)
        else:
            # Simple progress bar
            dl = 0
            total_length = int(total_length)
            for chunk in response.iter_content(chunk_size=8192):
                dl += len(chunk)
                f.write(chunk)
                done = int(50 * dl / total_length)
                sys.stdout.write(f"\r[{'=' * done}{' ' * (50-done)}] {int(dl/total_length*100)}%")
                sys.stdout.flush()
    print()

def extract_tar(tar_path, dest_dir):
    print(f"Extracting {tar_path} to {dest_dir}...")
    with tarfile.open(tar_path, "r:gz") as tar:
        tar.extractall(path=dest_dir)

def ensure_slint_sdk(env, lib_dir):
    target_arch = get_target_arch(env)
    sdk_name = f"Slint-cpp-{SLINT_VERSION}-{target_arch}"
    sdk_dir = lib_dir / "sdk" / sdk_name

    sdk_dir.mkdir(parents=True, exist_ok=True)

    print(f"[Slint_LovyanGFX] Checking Slint SDK in {sdk_dir}...")
    
    # If already installed, skip
    if (sdk_dir / "include"/ "slint" / "slint.h").exists():
        return sdk_dir

    url = f"https://github.com/slint-ui/slint/releases/download/v{SLINT_VERSION}/{sdk_name}.tar.gz"
    tar_path = sdk_dir / f"{sdk_name}.tar.gz"
    
    download_file(url, tar_path)
        
    extract_tar(tar_path, sdk_dir.parent)
    
    # Cleanup tar
    if tar_path.exists():
        os.remove(tar_path)
        
    return sdk_dir

def ensure_slint_compiler(lib_dir):
    tools_dir = lib_dir / "tools"
    tools_dir.mkdir(parents=True, exist_ok=True)
    
    host_platform = get_host_platform()
    compiler_name = f"slint-compiler-{host_platform}"
    
    # Executable name
    exe_name = "slint-compiler.exe" if platform.system() == "Windows" else "slint-compiler"
    compiler_path = tools_dir / exe_name
    
    if compiler_path.exists():
        return compiler_path

    url = f"https://github.com/slint-ui/slint/releases/download/v{SLINT_VERSION}/{compiler_name}.tar.gz"
    tar_path = tools_dir / f"{compiler_name}.tar.gz"
    
    download_file(url, tar_path)
        
    extract_tar(tar_path, tools_dir)
    
    # Cleanup tar
    if tar_path.exists():
        os.remove(tar_path)
        
    return compiler_path

def compile_slint_file(source, target, env):
    # This function is called by SCons
    slint_file = Path(str(source[0]))
    output_header = Path(str(target[0]))
    
    # We need to find the compiler again or pass it via env
    # For simplicity, we resolve it again (it should be fast)
    lib_dir = Path(env.get("SLINT_LIB_DIR"))
    compiler_path = ensure_slint_compiler(lib_dir)
    
    output_header.parent.mkdir(parents=True, exist_ok=True)
    
    cmd = [
        str(compiler_path),
        str(slint_file),
        "--embed-resources", "embed-for-software-renderer",
        "-I", str(env.get("PROJECT_DIR")) + "/src/ui", # Include path for imports
        "-o", str(output_header)
    ]
    
    print(f"[Slint_LovyanGFX] Compiling {slint_file.name}...")
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode != 0:
        sys.stderr.write(f"Slint compilation failed:\n{result.stderr}\n")
        env.Exit(1)

def configure_env(env):
    import inspect
    try:
        script_path = Path(inspect.getfile(inspect.currentframe()))
    except:
        script_path = Path(__file__)
    lib_dir = script_path.parent
    env["SLINT_LIB_DIR"] = str(lib_dir)
    sdk_dir = ensure_slint_sdk(env, lib_dir)

    # Include Paths
    env.Append(CPPPATH=[
        str(sdk_dir / "include"),
        str(sdk_dir / "include" / "slint")
    ])
    env.Append(LIBPATH=[str(sdk_dir / "lib")])
    env.Append(LIBS=["slint_cpp"])
    
    # Linker Script (esp-println.x)
    # This is needed for Rust panic handling on ESP
    # See: https://github.com/slint-ui/slint/blob/e41a460b135175a570e39c95985ded246e368bfb/api/cpp/esp-idf/slint/esp-println.x
    ld_script = lib_dir / "esp-println.x"
    if ld_script.exists():
        env.Append(LIBPATH=[str(lib_dir)])
        # Use -T to specify linker script
        env.Append(LINKFLAGS=[f"-T{ld_script.name}"])

    # === Add .slint files Compile Command ===
    # generate headers in .pio/build/<env>/Slint_LovyanGFX_generated/
    # then it can be clean by PIO automatically
    project_dir = Path(env.subst("$PROJECT_DIR"))
    build_dir = Path(env.subst("$BUILD_DIR"))
    generated_dir = build_dir / "Slint_LovyanGFX_generated"
    # Add generated dir to include path for everyone
    # then it can be included as #include "my_ui.h" 
    env.Append(CPPPATH=[str(generated_dir)])

    ui_dir = project_dir / "src" / "ui"
    if ui_dir.exists():
        slint_files = list(ui_dir.rglob("*.slint"))
        generated_headers = []
        
        for slint_file in slint_files:
            header_name = slint_file.with_suffix(".h").name
            header_file = generated_dir / header_name
            
            # We use the library env to define the command, but it needs to run before main build.
            # Adding it as a dependency to the library or global build.
            
            env.Command(
                str(header_file),
                str(slint_file),
                compile_slint_file
            )
            generated_headers.append(str(header_file))
            
        # Force generation before compiling source files
        # We can attach it to the compilation of the library itself, 
        # or try to attach to main.cpp if we can find it.
        # Attaching to the library's build target is safest.
        # But since main.cpp includes the header, SCons should detect the dependency 
        # IF we tell SCons that main.cpp depends on these headers.
        # But SCons scanner might not find the header if it doesn't exist yet.
        # So we explicitly add it.
        
        # Add to global build dependencies
        # This ensures they are built before the final executable
        env.Depends("$PROGPATH", generated_headers)
