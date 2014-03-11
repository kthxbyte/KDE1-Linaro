#!/bin/sh
# This is the installation script for KDE, packaged in .tgz form for Slackware.
#
# Add the KDE library path to /etc/ld.so.conf:
if fgrep /opt/kde/lib etc/ld.so.conf 1> /dev/null 2> /dev/null ; then
  # Good, we've already found the KDE path in etc/ld.so.conf, so we
  # won't need to add it again.
  FOUND=GOOD # :^)
else
  echo "/opt/kde/lib" >> etc/ld.so.conf
fi
