#! /usr/bin/env python
# -*- coding:utf-8 -*-

from scipy.io import wavfile
import numpy as np


def _main(args):
    nrepeat = int(args.sample_rate * args.frame_size_ms * 0.001)
    with open(args.inf, 'r') as f:
        data = np.array([[float(y) for y in x.strip().split()] for x in f])
        data = data.repeat(nrepeat, axis=0)
        data = (data - .5) * 2
        data = np.clip(data, -0.99 , 0.99)
        wavfile.write(args.outf, args.sample_rate, data)


if __name__ == '__main__':
    import argparse
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('inf', type=str)
    arg_parser.add_argument('outf', type=str)
    arg_parser.add_argument('--sample_rate', type=int, default=16000)
    arg_parser.add_argument('--frame_size_ms', type=int, default=10)
    args = arg_parser.parse_args()
    _main(args)
