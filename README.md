# GRAM

A lightweight text editor based on https://viewsourcecode.org/snaptoken/kilo/index.html written in C. 
Why? To use it to make quick changes on file on servers i do not have vim installed

## Shortcuts
* `CTRL + D`: duplicate line
* `CTRL + R`: remove line
* `CTRL + T`: indent line
* `CTRL + P`: step one word left
* `CTRL + N`: step one word right

## Progress:
- [ ] line numbers
- [ ] libncurses?
- [ ] split source in multiple files
- [ ] remove all break in loop statement (i do not like them)
- [ ] handle F1 - F10?
- [ ] config file
- [ ] auto indent
- [ ] syntax highlight
- [ ] ctrl z -> undo 
- [ ] ctrl y -> redo
- [X] ctrl d -> duplicate line
- [X] ctrl r -> remove line
- [X] ctrl t -> indent line
- [X] ctrl j -> remove indentation
- [ ] ctrl c -> copy current word
- [ ] ctrl v -> paste buffer (copy) content
- [ ] display buffer content in status bar
- [ ] ctrl h -> comment current line
- [ ] file extension detection used to comment line
- [ ] fix saving files
- [X] enable move 1 word (ctrl arrow)
- [ ] comment line (detect file extension)

## Note
If something bad happens (errors that close the application), the terminal will be compromised. 
Just type `reset` (it will not be displayed in the terminal) to restore the default terminal settings.
If you want to compile for the release (not debugging), change the first line of Makefile to `DEBUG=0`.