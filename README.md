foo_output_pipe
===============

Component for foobar2000 music player.

Pipe played audio to console app. i.e for streaming or saving audio.

As soon as you start playback in foobar2000 it will launch the program and pipe audio to it.

Latest build:
https://saivert.com/files/foo_output_pipe.7z


![Screenshot](http://i.imgur.com/3FAgMra.png)

Older Screenshot

![Older screenshot](http://i.imgur.com/GoMYBXY.png)

Build instructions

1. Download foobar2000 SDK (http://www.foobar2000.org/SDK) and extract it somewhere.

2. Clone this repo and place it in the foobar2000 SDK's foobar2000 directory.

3. Open the solution file in Visual Studio.

4. Download wtl (http://wtl.sourceforge.net/) and extract it somewhere.

5. Edit foo_output_pipe project settings and fix include path to point to wtl's Include directory. Do the same for foobar2000_ATL_helpers.

6. Open project properties for foo_output_pipe and under Build events - Post-build event fix the path to the portable fb2k installation you use (or should use) for debugging.

