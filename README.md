# ESP32 Projects

Maybe Docker is overkill. And, eventually, the source code for the projects should be in separate
repositories. But, for now, everything is in here.

## Base

In the root directory is the esp-idf base. The projects should use the Dockerfile from this. It will
be built for the specific version.

# Subdirectories

So far, I've just done blink. But, if it's good enough for the examples, it's a good starting point. It
will use the specific version of the base esp-edf image. You build the image, then run it. You need to:

```
% idf.py set-target esp32 # or whatever
% idf.py build # if the defaults are okay
% idf.py -p /dev/ttyUSB0 flash # that's likely your port, but you should check first.
