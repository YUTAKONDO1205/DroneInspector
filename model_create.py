from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent
TFLITE_PATH = PROJECT_ROOT / "crack_classifier_mobilenetv2.tflite"
MODELS_DIR = PROJECT_ROOT / "models"
HEADER_PATH = MODELS_DIR / "model_data.h"
CPP_PATH = MODELS_DIR / "model_data.cpp"


HEADER_TEXT = """#pragma once

#include <stdint.h>

// TFLite model byte array
extern const unsigned char g_model_data[];
extern const unsigned int g_model_data_len;
"""


def build_model_sources(tflite_path: Path = TFLITE_PATH) -> int:
    if not tflite_path.exists():
        raise FileNotFoundError(f"Not found: {tflite_path}")

    MODELS_DIR.mkdir(parents=True, exist_ok=True)

    data = tflite_path.read_bytes()
    model_len = len(data)

    HEADER_PATH.write_text(HEADER_TEXT, encoding="utf-8")

    with CPP_PATH.open("w", encoding="utf-8") as f:
        f.write('#include "model_data.h"\n\n')
        f.write(f"// Auto-generated from {tflite_path.name}\n")
        f.write("const unsigned char g_model_data[] = {\n")

        for i, byte in enumerate(data):
            if i % 12 == 0:
                f.write("  ")
            f.write(f"0x{byte:02x}")
            if i != model_len - 1:
                f.write(", ")
            if i % 12 == 11:
                f.write("\n")

        if model_len % 12 != 0:
            f.write("\n")

        f.write("};\n\n")
        f.write(f"const unsigned int g_model_data_len = {model_len};\n")

    return model_len


def main() -> None:
    model_len = build_model_sources()
    print("Generated:")
    print(f"  {HEADER_PATH}")
    print(f"  {CPP_PATH}")
    print(f"Model size: {model_len} bytes")


if __name__ == "__main__":
    main()
