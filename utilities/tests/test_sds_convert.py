# SPDX-License-Identifier: Apache-2.0

import importlib.util
import struct
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).resolve().parents[1] / "sds-convert.py"

spec = importlib.util.spec_from_file_location("sds_convert", MODULE_PATH)
sds_convert = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sds_convert)


class PrepareDataTests(unittest.TestCase):
    def test_mixed_value_types_are_decoded_per_sample(self):
        meta_data = [
            {"value": "confidence", "type": "double"},
            {"value": "x", "type": "uint32_t"},
            {"value": "y", "type": "uint32_t"},
            {"value": "w", "type": "uint32_t"},
            {"value": "h", "type": "uint32_t"},
        ]

        expected = [
            (0.7162540555000305, 143, 9, 40, 55),
            (0.5, 10, 20, 30, 40),
        ]

        raw_data = b"".join(
            struct.pack("=dIIII", *sample)
            for sample in expected
        )

        decoded = sds_convert.prepareData(
            meta_data,
            raw_data,
            data_manipulation=False,
        )

        actual = list(zip(*decoded))
        self.assertEqual(expected, actual)

    def test_equal_sized_types_keep_existing_behavior(self):
        meta_data = [
            {"value": "x", "type": "uint32_t"},
            {"value": "y", "type": "uint32_t"},
        ]

        expected = [
            (1, 2),
            (3, 4),
            (5, 6),
        ]

        raw_data = b"".join(
            struct.pack("=II", *sample)
            for sample in expected
        )

        decoded = sds_convert.prepareData(
            meta_data,
            raw_data,
            data_manipulation=False,
        )

        actual = list(zip(*decoded))
        self.assertEqual(expected, actual)

    def test_scale_and_offset_are_applied_per_channel(self):
        meta_data = [
            {
                "value": "temperature",
                "type": "int16_t",
                "scale": 0.5,
                "offset": 10,
            },
            {
                "value": "pressure",
                "type": "uint32_t",
                "scale": 2,
                "offset": -5,
            },
        ]

        raw_data = b"".join(
            [
                struct.pack("=hI", 20, 100),
                struct.pack("=hI", -4, 50),
            ]
        )

        decoded = sds_convert.prepareData(
            meta_data,
            raw_data,
            data_manipulation=True,
        )

        self.assertEqual(
            [
                [20.0, 8.0],
                [195, 95],
            ],
            decoded,
        )


if __name__ == "__main__":
    unittest.main()
