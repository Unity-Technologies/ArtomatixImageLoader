#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mono $DIR/paket.bootstrapper.exe 4.5.0
