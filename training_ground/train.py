#!/usr/bin/python

import random
import sys
import os
from suites import suites
from src import options
from src import config
from src.reinforcer import Reinforcer

import logging

logging.basicConfig()
logging.getLogger().setLevel(logging.DEBUG)

def main():
    options.parse()
    reinforcer = Reinforcer(suites.get_suites())
    reinforcer.measure_baseline('-O2')
    reinforcer.run()
    print(config.a)

if __name__ == "__main__":
    main()
