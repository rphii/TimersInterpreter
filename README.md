# TODO TO BE LEAK FREE
- wherever I call freeing functions, also call them in the ERROR section of code

# Timers
Timers is an esoteric programming language.

## Story
Original concept was created back in 2021 by me (rphii).

## Memory Model
Stack based

## Idea
The idea is that *Timers* relies on "time" - no need to worry about incrementing anything, because time will do it for you!

## Example Programs
See TODO (add link) for a list of instructions

### Hello World
```
(['Hello, World!\n']~)
-(^,~)
```
Explanation:
- first of all, there is one function with the time of activation at `0` (value before first `(`, which is empty)
    1. `['Hello, World!\n']` for each character, start a timer
    2. `~` destroy caller's timer
- secondly, there is a function that takes any time (`-` is a range. since empty, assume from `0` to the equivalent of `-1` unsigned)
    1. `^` push caller's time to top of stack
    2. `,` print top of stack as a character
    3. `~` destroy caller's timer

### [Truth Machine](https://esolangs.org/wiki/Truth-machine)
```
(@?~)1{}
```
Explanation:
- there is one function with the time of activation at `0` (value before first `(`, which is empty)
    1. `@` get a string from the user, store in top of stack
    2. `?` insert the top of stack into code. if it is a valid character, search for a matching definition and execute it
    3. `~` destroy the caller's timer (stop program)
- there is one definition with the name `1`
    1. it gets called if the user enters a `1`
    2. the scope is empty, which automatically results in an infinite loop as soon as it is entered

### Experimental Truth Machine
```
(~&!|{})
```

### Calculator
Takes an integer, an operation (`+`, `-`, `*` or `/`), a second integer and outputs the result
```
(&&&\?."~)
```
Explanation:
- there is one function with the time of activation at `0` (value before first `(`, which is empty)
    1. get three integers from the user (`&&&`), store in top of stack. a non-integer is converted to it's unicode codepoint
    2. `\` swap top of stack with below top of stack
    3. `?` insert top of stack into code (an operation) and execute it
    4. `.` print top of stack as integer
    5. `"` print a new line
    6. `~` destroy the caller's timer (stop program)

### *experimental* Fizz-Buzz **TODO link**
```
+3(^{(['Fizz'])-(^,~)})
+5(^{(['Buzz'])-(^,~)})
-(+|.)
-($")
```

## The Rules

### Operations
The following operations can be put inside any function of time **TODO link**.

|Character  |Description|
|-----------|-----------|
|`~`        |Destroy caller's time (finish execution of current function)|
|`^`        |Push caller's time|
|`$`        |Pop top of stack|
|`\`        |Swap top of stack with below top of stack|
|`:`        |Push top of stack|
|`;`        |Push size of stack|
|`.`        |Pop top of stack and output as integer|
|`,`        |Pop top of stack and output as character|
|`"`        |Print a newline|
|`#`        |Pop top value, n, and push n-th stack value (bottom = 0). If the stack is smaller than the given index, skip|
|`` ` ``    |Pop two values, n (top), b, overwrite n-th stack value (bottom = 0) with b. If the stack is smaller than the given index, skip|
|`?`        |Pop top of stack and insert character into code, if it's a valid unicode character. Else, insert as integer instead. If the stack is empty, insert an `_` instead. Using multiple `?` will automatically concenate them with whatever is before/after them. The resulting instruction is either one of the command list or a function call. And finally, if it results in an invalid instruction, skip|
|`&`        |Get an integer from the user and push it. If a string is entered it will convert each to its unicode codepoint and push that instead|
|`@`        |Get a string from the user and push it (latter characters in the string are pushed first|
|`+`        |Addition : Pop two values a and b, then push the result of a+b|
|`-`        |Subtraction : Pop two values a (top) and b, then push the result of b-a|
|`*`        |Multiplication : Pop two values a and b, then push the result of a times b|
|`/`        |Division : Pop two values a (top) and b, then push the result of b/a, rounded down|
|`%`        |Modulo : Pop two values a (top) and b, then push the remainder of the integer division of b/a|
|`>`        |Greater than : Pop two values a (top) and b, then push 1 if a&gt;b, otherwise 0|
|`<`        |Less than : Pop two values a (top) and b, then push 1 if a&lt;b, otherwise 0|
|`=`        |Equals : Pop two values a and b, then push 1 if a=b, otherwise 0|
|`!`        |Logical not : Pop a value. If the value is 0 push 1, otherwise 0|
|<code>&#124;</code>|If : Pop a value. If the value is not 0, exit the current function|
|`d`        |Execute a defined scope, where `d` gets replaced with the name (see scope **TODO link**)|
|`{f}`      |Execute a scope, where `f` gets replaced with function(s) of time. Similar to above, but inlined|
|`[t]`      |Initiate a new timer (finish execution of current function), starting from `t`, where `t` is a number (see numbers **TODO link**)|

### New
The following combinations can be but inside `[]` to create new timers.


|..|Description|
|---|---|
|`[.]`|Maximum value any timer can have|
|`[,]`|0 if stack is empty|
|`[?]`|Value at top of stack (not modifying)|
|`[!]`|Value below top of stack (not modifying)|
|`[$]`|Last destroyed timer's value (scope independent)|
|`[&]`|By how much any timer was incremented previously, if > 0|
|`[%]`|Insert previous values of new|
|`[^]`|Value at index by top of stack|
|`[@]`|Value at index by below top of stack|
|`[\]`|Top of stack if not 0|
|`[/]`|Maximum value any timer can have if stack is not empty| ???
|`[>]`|Number of timers if that is > stack length, else empty |
|`[<]`|Stack length if that is > number of timers, else empty |
|`[=]`|Number of timers running in the current scope|
|`["]`|Sum of all timers running in the current scope|
|`[:]`|Scope name|
|`[;]`|Scope depth (main scope = 0, increment when going deeper)|
|``[`]``|Count of how often timers have been incremented already|

```
all: _~^$\:;.,"#`?&@+-*/=%><!|'
res: _~                       '  // reserved due to reasons
seq:           #    +-*      |
new:   ^$\:;.," `?&@   /=%><!
```

# TimersInterpreter
