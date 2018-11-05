#!/usr/bin/env python3

import otf2
import os
import sys
import gzip
import numpy as np

def main():
    # with gzip.open(sys.argv[1], 'rb') as infile:
    with open(sys.argv[1], 'rb') as infile:
        version = np.fromfile(infile, np.int32, 1)[0]
        print("Version: {}".format(version))

        start_time_s = np.fromfile(infile, np.uint64, 1)[0]
        start_time_us = np.fromfile(infile, np.uint64, 1)[0]
        start_time = start_time_s + start_time_us / 1e6
        print("Start: {}".format(start_time))
        rate = np.fromfile(infile, np.int32, 1)[0]
        print("Rate: {}".format(rate))

        gain_vdd = 300
        resistor_vdd = 0.00333

        num_channels = np.fromfile(infile, np.int32, 1)[0]
        print("Num channels: {}".format(num_channels))
        row_type = np.dtype((np.float32, num_channels))

        ticks = 0
        with otf2.writer.open("trace", timer_resolution=int(rate)) as trace:
            m_trigger = trace.definitions.metric("Trigger")
            m_vdd = trace.definitions.metric("Power Vdd", "W")
            m_vpp = trace.definitions.metric("Power Vpp", "W")
            writer = trace.event_writer("Master")

            data = np.fromfile(infile, dtype=row_type)
            for points in data:
                trigger = points[2]
                if np.isinf(points).any():
                    break
                vdd = ((points[0] / gain_vdd) / resistor_vdd) * points[1]

                writer.metric(ticks, m_vdd, vdd)
                writer.metric(ticks, m_trigger, trigger)

                ticks += 1

if __name__ == '__main__':
    main()