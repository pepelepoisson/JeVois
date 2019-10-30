Notes:
- Code Nunchi_V1 to be used on arduino with TFT display.
- Hardware specific libraries are required. See specific folder.
- file params.cfg to be copied onto JeVois SD card under /config/ to set the serial port name and baud rate.
- file videomappings.cfg to be loaded to JeVois camera via the JeVoisInventor program. Its only purpose is to define the settings corresponding to the Nunchi project that was originaly created via JeVoisInventor Add New Python module interface.
- file Nunchi.py contains the Python code to be loaded as part of the newly created Nunchi JeVois module. It is based on PyEmotion example with changes to export results to serial and run without USB connection.
- file initscript.cfg to be loaded to JeVois camera true the JeVoisInventor program. It is used to ensure JeVois starts automatically the Nunchi code in NoUSB mode when powered.

