#!/usr/bin/env python3

"""
Reference model for the streaming audio feature extractor project

This script:
1. Generates several test frames
2. Computes expected features for each frame
3. Saves:
   - input samples
   - expected outputs

Features:
- Energy: sum(x[n]^2)
- ZCR: zero-crossing count

Outputs are written to:
- python/test_vectors.json
- python/test_vectors.txt
"""

from __future__ import annotations

import json
import math
import random
from pathlib import Path
from typing import Dict, List


FRAME_LENGTH = 32
RNG_SEED = 12345


def extract_features(frame: List[int]) -> Dict[str, int]:
    """
    Compute golden-model features for one frame.

    Args:
        frame: List of signed integer samples

    Returns:
        Dict with:
            - energy
            - zcr
    """
    energy = sum(x * x for x in frame)

    zcr = 0
    for i in range(1, len(frame)):
        if (frame[i - 1] >= 0 and frame[i] < 0) or (frame[i - 1] < 0 and frame[i] >= 0):
            zcr += 1

    return {
        "energy": energy,
        "zcr": zcr,
    }


def generate_all_zeros(length: int) -> List[int]:
    return [0] * length


def generate_all_positive(length: int, value: int = 1000) -> List[int]:
    return [value] * length


def generate_alternating(length: int, magnitude: int = 1000) -> List[int]:
    return [magnitude if i % 2 == 0 else -magnitude for i in range(length)]


def generate_sine_wave(length: int, amplitude: int = 1000, cycles: float = 2.0) -> List[int]:
    """
    Generate an integer sine wave over length samples.

    cycles = number of sine cycles across the frame
    """
    frame = []
    for n in range(length):
        value = amplitude * math.sin(2.0 * math.pi * cycles * n / length)
        frame.append(int(round(value)))
    return frame


def generate_random_noise(length: int, low: int = -1000, high: int = 1000) -> List[int]:
    return [random.randint(low, high) for _ in range(length)]


def build_test_vectors(length: int) -> List[Dict]:
    random.seed(RNG_SEED)

    test_frames = [
        {
            "name": "all_zeros",
            "samples": generate_all_zeros(length),
        },
        {
            "name": "all_positive",
            "samples": generate_all_positive(length, value=1000),
        },
        {
            "name": "alternating_pos_neg",
            "samples": generate_alternating(length, magnitude=1000),
        },
        {
            "name": "sine_wave",
            "samples": generate_sine_wave(length, amplitude=1000, cycles=2.0),
        },
        {
            "name": "random_noise",
            "samples": generate_random_noise(length, low=-1000, high=1000),
        },
    ]

    results = []
    for test in test_frames:
        features = extract_features(test["samples"])
        results.append(
            {
                "name": test["name"],
                "frame_length": length,
                "samples": test["samples"],
                "expected": features,
            }
        )

    return results


def write_json(vectors: List[Dict], out_path: Path) -> None:
    with out_path.open("w", encoding="utf-8") as f:
        json.dump(vectors, f, indent=2)


def write_text(vectors: List[Dict], out_path: Path) -> None:
    with out_path.open("w", encoding="utf-8") as f:
        for vec in vectors:
            f.write(f"TEST: {vec['name']}\n")
            f.write(f"FRAME_LENGTH: {vec['frame_length']}\n")
            f.write(f"SAMPLES: {vec['samples']}\n")
            f.write(f"EXPECTED_ENERGY: {vec['expected']['energy']}\n")
            f.write(f"EXPECTED_ZCR: {vec['expected']['zcr']}\n")
            f.write("\n")


def print_summary(vectors: List[Dict]) -> None:
    print("Generated test vectors:\n")
    for vec in vectors:
        print(f"{vec['name']}:")
        print(f"  samples   = {vec['samples']}")
        print(f"  energy    = {vec['expected']['energy']}")
        print(f"  zcr       = {vec['expected']['zcr']}")
        print()


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    json_path = script_dir / "test_vectors.json"
    text_path = script_dir / "test_vectors.txt"

    vectors = build_test_vectors(FRAME_LENGTH)

    write_json(vectors, json_path)
    write_text(vectors, text_path)
    print_summary(vectors)

    print(f"Saved JSON test vectors to: {json_path}")
    print(f"Saved text test vectors to: {text_path}")


if __name__ == "__main__":
    main()