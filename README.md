# tty-bat

Terminal-based battery indicator

# about

`tty-bat` is a small utility that displays a graphical indication of
battery charge in a Linux terminal.


# usage

Clone the repository and run `make`. Requires ncurses.

```
tty-bat [args]

-b          blink the battery when battery level is below 10%
-B          use bold colors
-c          set the color
-h          show program usage
-l          set the color used when battery level is below 10%
-L num      set the battery level considered 'low' (default 10)
-N          don't draw battery percentage
-t RATE     set the update rate to RATE milliseconds
-x width    set the battery width to X lines
-y height   set the battery height to Y lines
-v          show program version information
```
