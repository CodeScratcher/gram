# GRAM

A lightweight text editor based on https://viewsourcecode.org/snaptoken/kilo/index.html written in C. 
Why? To use it to make quick changes on file on servers i do not have vim installed

## Shortcuts
* `CTRL + A`: not assigned (select all)
* `CTRL + B`: move to the beginning of the line
* `CTRL + C`: copy current word
* `CTRL + D`: duplicate line
* `CTRL + E`: move to the end of the line
* `CTRL + F`: find
* `CTRL + G`: not assigned
* `CTRL + H`: move line up
* `CTRL + I`: comment line
* `CTRL + J`: remove indentation
* `CTRL + K`: not assigned
* `CTRL + L`: not assigned
* `CTRL + M`: not assigned
* `CTRL + N`: step one word right
* `CTRL + O`: not assigned
* `CTRL + P`: step one word left
* `CTRL + Q`: quit
* `CTRL + R`: remove line
* `CTRL + S`: save
* `CTRL + T`: indent line
* `CTRL + U`: uncomment line
* `CTRL + V`: paste the content of the buffer
* `CTRL + W`: move line up
* `CTRL + X`: not assigned (cut line)
* `CTRL + Y`: not assigned (redo)
* `CTRL + Z`: not assigned (undo)

## Progress:
- [ ] line numbers
- [ ] libncurses?
- [ ] split source in multiple files
- [ ] remove all break in loop statement (i do not like them)
- [ ] handle F1 - F10?
- [ ] config file
- [ ] auto indent
- [ ] syntax highlight
- [ ] fix saving files
- [ ] show all shortcuts with ctrl l (maybe not)
- [ ] remove include from header
- [ ] ctrl z -> undo 
- [ ] ctrl y -> redo
- [X] ctrl d -> duplicate line
- [X] ctrl r -> remove line
- [X] ctrl t -> indent line
- [X] ctrl j -> remove indentation
- [X] ctrl c -> copy current word
- [X] ctrl v -> paste buffer (copy) content
- [X] ctrl b -> move to beginning of the line
- [X] ctrl e -> move to the end of the line
- [X] display buffer content in status bar
- [X] ctrl i -> comment current line
- [X] ctrl u -> uncomment current line
- [X] file extension detection used to comment line
- [X] enable move 1 word (ctrl arrow)

## Note
If something bad happens (errors that close the application), the terminal will be compromised. 
Just type `reset` (it will not be displayed in the terminal) to restore the default terminal settings.
If you want to compile for the release (not debugging), change the first line of Makefile to `DEBUG=0`.