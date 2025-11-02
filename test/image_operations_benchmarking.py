import os
import cv2
import time
import csv

# ==============================
# Image Operation Helpers
# ==============================
def apply_threshold(img, t1, t2):
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    _, th = cv2.threshold(gray, t1, t2, cv2.THRESH_BINARY)
    return th

def adjust_brightness(img, value):
    return cv2.convertScaleAbs(img, alpha=1.0, beta=value)

def adjust_contrast(img, value):
    return cv2.convertScaleAbs(img, alpha=value, beta=0)

def convert_to_greyscale(img):
    return cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

# ==============================
# Benchmarking Function
# ==============================
def benchmark_images(input_folder, output_csv):
    operations = [
        ("threshold", lambda img: apply_threshold(img, 100.3, 50.999)),
        ("brightness", lambda img: adjust_brightness(img, 100)),
        ("contrast", lambda img: adjust_contrast(img, 2.0)),
        ("greyscale", lambda img: convert_to_greyscale(img))
    ]

    images = [f for f in os.listdir(input_folder)
              if f.lower().endswith((".jpg", ".jpeg", ".png"))]

    if not images:
        print(f"⚠️ No images found in {input_folder}")
        return

    # -----------------------------
    # Build CSV Header
    # -----------------------------
    header = ["Image"] + [f"{op}_time(ms)" for op, _ in operations]

    with open(output_csv, mode="w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(header)

        for img_name in images:
            img_path = os.path.join(input_folder, img_name)
            row = [img_name]

            print(f"\n🧩 Benchmarking {img_name} ...")

            for op_name, op_func in operations:
                t0 = time.perf_counter()
                img = cv2.imread(img_path)
                if img is None:
                    print(f"❌ Failed to load {img_name}")
                    row.append("N/A")
                    continue

                out = op_func(img)
                output_path = os.path.join(input_folder, f"output_{op_name}_{img_name}")
                cv2.imwrite(output_path, out)
                t1 = time.perf_counter()

                total_time_ms = (t1 - t0) * 1000
                print(f"   {op_name:<12} total={total_time_ms:.2f} ms")

                row.append(f"{total_time_ms:.2f}")

            writer.writerow(row)

    print(f"\n✅ Benchmark results saved to {output_csv}")

# ==============================
# Entry Point
# ==============================
if __name__ == "__main__":
    input_folder = "/workspace/dsl-image-processing/build/testing_images"
    output_csv = os.path.join(input_folder, "python_benchmark_flat.csv")
    benchmark_images(input_folder, output_csv)
