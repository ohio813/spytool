This tiny app allows you to spy for others activity on their/your computer.
It's links only win API and has very small footprint.
Currently it allows to:
  * grab video from webcam only when there is a motion (currently only AVI is supported)
  * log all keystrokes (keyboard layout, languages support)
  * dump screens of active programs (THIS FEATURE IS NOT IMPLEMENTED YET!)
  * You can define disk space usage quota which will never be exceeded. Old files are deleted automatically.

The report is prepared as HTML page containing all the collected data in a timeline.
Future features:
  * thin web server allowing access to the generated HTML logs including all the data (keystrokes, video files and screenshots)
  * sending log via email

As I do not have too much time for working on this project, I'm inviting anyone who want to contribute to SpyTool. Remember, this app uses WinAPI calls only without linking any libraries.

-- Greg