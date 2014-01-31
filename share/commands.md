
##Commands

Alopex commands are made up of strings of *words*.

Each *word* is a string of non-space characters that represents a single
action.  A word can specify an optional target window an action and any
optional parameter.

Target windows are specified by starting the word with a *w* followed by
a number from 0 to 9 to specify a marked window.  In the absence of a
targetted window, the currently focused client is used as a default
target.  Some actions will behave differently if provided with the
focused window as a target or provided no explicit target (e.g., see tag
actions). A *w1* followed by an action and *w* followed by the action
are equivalent: they specify the currently focused window as an explicit
target.

**Display** Set/reset margins or focus a specified monitor

>dm#,#,#,#

Sets the margins to the numbers specified for a margin on the
top,bottom,left,right

>d#

Focuses the monitor specified (counting starts from 1)

**Focus** Sets focus to a client window

>w#f

Focuses the window at the specified mark number

>fh fj fk fl

Focuses the first,next,previous,last window currently displayed

**Move** Move a window in the stack

>w#mb w#ma w#ms

Moves the focused window relative to the window at the specified mark,
moves focused window before or after the marked window, or swaps the
marked and focused windows

>mh mj mk ml

Moves the focused window to the top,up,down,bottom of the client stack

**Tag** Control tag display and window occupancy

>w#tx# w#tt# w#ta# w#tr#

Modify the tag placement of the specified window to be
exclusive,toggle,add,remove the specified tag number

>tx# tt# ta# tr#

Modify the tag visibility to exclusive,toggle,add,remove the specified
tag number to the current monitor

**Layout** Select a tiling layout

>lr lb lm lx

Select rstack, bstack, monocle, or cycle through modes





