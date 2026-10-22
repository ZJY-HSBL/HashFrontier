#!/usr/bin/env python3
"""Regression tests: native CI must not accept incomplete experiment output."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from audit_zombie_native import audit_log, parse_protocol

PROTOCOL = 'q=13;load=95;cycles=2;deletions=2;insertions=2;lookups=4;unpaired=true'
HEADER = 'churn_cycle  ts  duration   num_ops  false_lookups total_lookups false_delete total_delete total_inserts op'
LINES = [
    '0 10 30 2 0 0 0 2 0 DELETE',
    '0 20 40 2 0 0 0 2 2 INSERT',
    '0 30 50 4 0 4 0 2 2 LOOKUP',
    '1 40 30 2 0 4 0 4 2 DELETE',
    '1 50 40 2 0 4 0 4 4 INSERT',
    '1 60 50 4 0 8 0 4 4 LOOKUP',
]

class AuditTests(unittest.TestCase):
    def test_complete(self):
        info, records = audit_log(PROTOCOL, HEADER+'\n'+'\n'.join(LINES), 'GZHM', 'test')
        self.assertTrue(info['complete'])
        self.assertEqual(info['completed_cycles'], 2)
        self.assertEqual(len(records), 6)
        self.assertEqual(records[-1]['clock_ticks_per_op'], 12.5)

    def test_exit_zero_does_not_imply_complete(self):
        info, records = audit_log(PROTOCOL, HEADER+'\n'+'\n'.join(LINES[:-2]), 'TRHM', 'test')
        self.assertFalse(info['complete'])
        self.assertEqual(info['completed_cycles'], 1)
        self.assertEqual(info['trailing_partial_phases'], 1)
        self.assertEqual(info['stopped_during'], 'INSERT')
        self.assertEqual(len(records), 3)

    def test_corrupt_phase_rejected(self):
        corrupted = LINES.copy()
        corrupted[1] = corrupted[1].replace('INSERT', 'LOOKUP')
        with self.assertRaises(ValueError):
            audit_log(PROTOCOL, HEADER+'\n'+'\n'.join(corrupted), 'GZHM', 'test')

    def test_missing_protocol_rejected(self):
        with self.assertRaises(ValueError):
            parse_protocol('load=95;cycles=320')

if __name__ == '__main__':
    unittest.main()
