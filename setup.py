# just proxy to bindings/python/setup.py becasue pip is stupid and can't install from subdirs even though it says it can

import sys
import os

setupPyDir = os.path.dirname(os.path.realpath(__file__)) + "/bindings/python"
sys.path.insert(0, setupPyDir)

from setup import *
