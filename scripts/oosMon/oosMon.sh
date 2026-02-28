#!/bin/bash
filesToCopy="${HOME}/.oosMon/oosSumm.html ${HOME}/.oosMon/html_oos/"
copyDestination=/mnt/l/tickMon/webmon/
python oosDf.py && python oosHtml.py && sudo cp -r $filesToCopy $copyDestination
