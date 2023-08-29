# Timers
Timers is an esoteric programming language.

## Installation
The easiest way to install this is by running `make`. Pro-tip, to speed up compilation: You can
specify the number of threads via `make -j4`, where in this case you would specify that you have 4
threads.

### Compile flags
- `-DCOLORPRINT_DISABLE` add this to the `CFLAGS` in the Makefile to disable any form of colored or
  formatted (bold / colored) output of text.

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

### Fizz-Buzz **TODO link**
```
3+(^{(['Fizz'])-(^,~)})
5+(^{(['Buzz'])-(^,~)})
1-(+|^.)
1-($")
100(~)
```

### More Examples
See [examples](examples) for more examples.

## The Rules

### Comments
There are three types of ways to make comments.
1. The easiest one, but the riskiest one is that any invalid code is treated as a comment.
2. The second slightly less riskier version involves the unique trait of named [scopes](#Scopes]). Those types of comments are quite situational. You can treat a string as a comment, denoted by surrounded apostrophes (those guys: `'`). _We're skipping quite a bit, maybe come back later if you understood scopes._
	- One example is this: `'this is a comment'`. Beware, that if nothing except whitespace is between the end of the "comment" and a `{}` automatically assigns that comment as that scopes' name.
	- A more failsafe version might be this: `'this is a comment{}'`, and the arguably failsafest version: `'this is a comment{(~)}`, in case we accidentally enter the scope by "calling the comment" and being stuck in an infinite loop, if no timers get destroyed.
	- However, one caviat of this commenting technique is that you can't have two equal comments within the same scope. That changes however within a function of time, where you can have multiple equal comments and you can even let the `{(~)}` part away, since they will still be executed and we try to search for a matching named scope to enter. And if there is none, we simply continue.
	- Also, you have to adhere to the fact that there are escape sequences and that they have to be correct.
- The safest way to comment out things is by taking advantage of the fact that it doesn't make sense to destroy one timer two times in succession, we're talking of the [operation](#Operations|operation]) `~`. So, by using two `~~` right next to each other, anything on that line afterwards is treated as a comment. Note that unlike in the programming language C, you can't escape the end of line with a `\`, so you have to effectively mark each line as a comment, if you wish to do so.

### Time and related
- At program startup there is one timer whose value is initialized to 0.
- Every timer counts one up, until a corresponding [time function](#Time-Functions) is found, where they will pause until the code inside finished executing.
- Once a timer overflows it resets to 0.
- Every scope is treated like a seperate program with "private" timers.
- Technical note: The interval between counting is undefined. Ideally it approaches t=0, because then the interpreter/compiler is optimized.

### Flow of Execution
- The program starts executing in the main scope.
- The main scope is any code outside of defined scopes.
- Only the timers of the current scope are able to count.
- Once a scope is terminated, flow of execution is returned to its parent scope.

### Operations
The following operations can be put inside any [time function](#Time-Functions]).

|Character  |Description|
|-----------|-----------|
|`~`        |Destroy caller's time (finish execution of current function)|
|`^`        |Push caller's time|
|`$`        |Pop top of stack|
|`\`        |Swap top of stack with below top of stack. Only executed if there are at least 2 values|
|`:`        |Push top of stack|
|`;`        |Push size of stack|
|`.`        |Pop top of stack and output as integer|
|`,`        |Pop top of stack and output as character|
|`"`        |Print a newline|
|`#`        |Pop top value, n, and push n-th stack value (bottom = 0). If the stack is smaller than the given index, skip|
|`` ` ``    |Pop two values, n (top), b, overwrite n-th stack value (bottom = 0) with b. If the stack is smaller than the given index, skip|
|`?`        |Pop top of stack and insert character into code, if it's a valid unicode character. Else, insert as integer instead. If the stack is empty, insert an `_` instead. Using multiple `?` will automatically concenate them with whatever is before/after them. The resulting instruction is either one of the command list or a function call. And finally, if it results in an invalid instruction, skip|
|`&`        |Get an integer from the user and push it. If a string is entered it will still try to convert numbers but it will also convert each character to its unicode codepoint and push that instead|
|`@`        |Get a string from the user and push it (latter characters in the string are pushed first, so that the beginning lies on the top)|
|`+`        |Addition : Pop two values a and b, then push the result of a+b|
|`-`        |Subtraction : Pop two values a (top) and b, then push the result of b-a|
|`*`        |Multiplication : Pop two values a and b, then push the result of a times b|
|`/`        |Division : Pop two values a (top) and b, then push the result of b/a, rounded down|
|`%`        |Modulo : Pop two values a (top) and b, then push the remainder of the integer division of b/a|
|`>`        |Greater than : Pop two values a (top) and b, then push 1 if a&gt;b, otherwise 0|
|`<`        |Less than : Pop two values a (top) and b, then push 1 if a&lt;b, otherwise 0|
|`=`        |Equals : Pop two values a and b, then push 1 if a=b, otherwise 0|
|`!`        |Logical not : Pop a value. If the value is 0 push 1, otherwise 0|
|<code>&#124;</code>|If : Pop a value. If the value is not 0, exit the current function (continue if 0)|
|`d`        |Execute a defined scope, where `d` gets replaced with the name (see [scopes](#Scopes]))|
|`{f}`      |Execute a scope, where `f` gets replaced with function(s) of time. Similar to above, but inlined|
|`[t]`      |Initiate a new timer (finish execution of current function), starting from `t`, where `t` is a number (see [new](#New))|

### New
The following combinations can be but inside `[]` to create new timers. New timers start searching
for matching [time functions](#Time-Functions) starting on the next one (prevents this: `([0]~)-(.~)` from turning into an endless loop).

| ..             | Description                                                                                 |
| -------------- | ------------------------------------------------------------------------------------------- |
| `[.]`          | Maximum value any timer can have                                                            |
| `[,]`          | 0 if stack is empty                                                                         |
| `[?]`          | Value at top of stack (not modifying)                                                       |
| `[!]`          | Value below top of stack (not modifying)                                                    |
| `[$]`          | Last destroyed timer's value (scope independent)                                            |
| `[&]`          | By how much any timer was incremented previously, if > 0                                    |
| `[%]`          | Insert previous values of new                                                               |
| `[^]`          | Value at index by top of stack                                                              |
| `[@]`          | Value at index by below top of stack                                                        |
| `[\]`          | Top of stack if not 0                                                                       |
| `[/]`          | Maximum value any timer can have if stack is not empty                                      |
| `[>]`          | Number of timers if that is > stack length, else empty                                      |
| `[<]`          | Stack length if that is > number of timers, else empty                                      |
| `[=]`          | Number of timers running in the current scope                                               |
| `["]`          | Sum of all timers running in the current scope                                              |
| `[:]`          | Scope name                                                                                  |
| `[;]`          | Scope depth (main scope = 0, increment when going deeper)                                   |
| ``[`]``        | Count of how often timers have been incremented already                                     |
| `['A string']` | Match : All of the characters, keeps the order. Supports C-style escape sequences and utf-8 |
| `[1234567890]` | Number : Any number you can think of. Supports C-style hex (`0x`) or octal (`0`) prefixes   |

#### Divide by Zero

Dividing (or modulo) by zero brings some consequences with it:

- Applies to: Current scope only
- It destroys all (to the scope known) spacetime, therefore code gets terminated immediately and all timers (within that scope) blend together.
- Realities are shuffled through and the sole remaining timer takes a random value out of all the possibilities within that scope.
- TL;DR Randomize. (The non-dramatic version)

### Modifiers
Modifiers can be used in [new](#New) or for [time functions](#Time-Functions).

| ..                  | Description          |
| ------------------- | -------------------- |
| <code>&#124;</code> | Concatenation        |
| `-`                 | Range                |
| `+`                 | Linear Sequence      |
| `*`                 | Exponential Sequence |
| `#`                 | Times                |

#### Concatenation
- Takes anything on either side and concatenates it with the thing before.

#### Range
- Takes anything on either side, even a string.
- If one of either of the side is a string, it will create a range for all characters, e.g. `'abc'-''` is synonymous with `'a'-'b'-'c'-''`. Empty strings are removed and the whole construct is reduced to whatever is left, so the previous range is _actually_ going to be `'a'-'b'-'c'`. Single characters are converted to their codepoint.
- If there is a chain of values, e.g. `1-3-5-7` it will automatically assume that the programmer wants this instead `1-3|3-5|5-7`
- A range of two values creates each value once, so `1-5` is actually `1|2|3|4|5`.
- If the left side is not specified, default to 0.
- If the right side is not specified, default to the max. count any timer can have.

#### Linear Sequence
- Takes anything on either side, even a string.
- If one of either of the side is a string, it will create a linear sequence for all characters, e.g. `'abc'+''` is synonymous with `'a'+'b'+'c'+''`. Empty strings are removed and the whole construct is reduced to whatever is left, so the previous range is _actually_ going to be `'a'+'b'+'c'`. Single characters are converted to their codepoint.
- If there is a chain of values, e.g. `1+3+5+7` it will automatically assume that the programmer wants this instead `1+3|3+5|5+7`
- A linear sequence is $$v_i=v_\text{base}+i\cdot v_\text{step}$$ where $v_i$ is the i-th value of the sequence, $v_\text{base}$ the first and $v_\text{step}$ the second value.
- Optionally, one can specify an inclusive upper limit with a range modifier, e.g. `1+3-10` is synonymous to `1|4|7|10`.
- Optionally, one can specify a times modifier, e.g. `1+3#10` is synonymous to `1|4|7|10|13|16|19|22|25|28`. (which will consider up to 10 values, as specified by that 10)
- You can't use both modifiers at the same time.
- If the left side is not specified, default to 0.
- If the right side is not specified, default to left item.

#### Exponential Sequence
- Takes anything on either side, even a string.
- If one of either of the side is a string, it will create an exponential sequence for all characters, e.g. `'abc'*''` is synonymous with `'a'*'b'*'c'*''`. Empty strings are removed and the whole construct is reduced to whatever is left, so the previous range is _actually_ going to be `'a'*'b'*'c'`. Single characters are converted to their codepoint.
- If there is a chain of values, e.g. `2*3*5*7` it will automatically assume that the programmer wants this instead `2*3|2*5|2*7`
- An exponential sequence is $$v_i=(v_\text{base}+i)^{v_\text{step}}$$ where $v_i$ is the i-th value of the sequence, $v_\text{base}$ the first and $v_\text{step}$ the second value.
- Optionally, one can specify an inclusive upper limit with a range modifier, e.g. `1*3-100` is synonymous to `1|8|27|64`.
- Optionally, one can specify a times modifier, e.g. `1+3#10` is synonymous to `1|8|27|64|125|216|343|512|729|1000`. (which will consider up to 10 values, as specified by that 10)
- You can't use both modifiers at the same time.
- If the left side is not specified, default to 0.
- If the right side is not specified, default to left item.

#### Times
- Takes anything on either side, even a string.
- If one of either of the side is a string, it will create a times calculation for all characters, e.g. `'abc'#''` is synonymous with `'a'#'b'#'c'#''`. Empty strings are removed and the whole construct is reduced to whatever is left, so the previous example is _actually_ going to be `'a'#'b'#'c'`. Single characters are converted to their codepoint.
- If there is a chain of values, e.g. `2#3#5#7` it will automatically assume that the programmer wants this instead `2#3|2#5|2#7`
- In this context, times is referring to multiplication of two values.
- If the left side is not specified, default to 0.
- If the right side is not specified, default to left item.

### Time Functions
- Use any of the special operations used within the section [new](#New) in combination with [modifiers](#Modifiers). However, the numbers shall not be placed in square brackets, but before a pair of round brackets, e.g.  `HERE()`. 
- Within the `()` is the actual function body, where commands ([operations](#Operations)) are executed character for character.
- You can have up to one line break (and any amount of horizontal whitespace) that a number is associated to a function of time. Otherwise, if no number can be associated to a function of time, default to 0.
- All matching [time function](#Time-Functions) are executed if any timer with it's current value matches any of it's number(s).

#### The only difference in numbers to the new operation
- [New](#New) operation: `[1 2]` is different from `[1|2]`. If we have no valid [modifiers](#Modifiers) between any two numbers, including literals or whitespace, the newest timer will be the rightmost. If we correctly concatenate them, the newest timer will be the leftmost. _This adheres to the rules of creating matches of strings: `['A string'])`_
- [time functions](#Time-Functions): `1 2()` is equivalent to the function of time `2()`, where the `1` gets discarded. If you _really_ want to either hit `1` or `2`, you have to concatenate those in this case: `1|2()`.

### Scopes
- Scopes are recognized by curly brackets: `{}`
- There are two types of scopes:
	- Inline scopes, that are always nameless and shall be within a function of time, e.g. `(IN HERE)`.
	- Named scopes, that are always named and shall be outside function of times, e.g. `OVER HERE ()`

#### Inline Scopes
- The quickest way to easily change scopes is by using inline scopes, as those are automatically entered.
- Entering any kind of scope starts a new timer, with the value of 0.
- Exiting any kind of scope happens when there is no timer left.
#### Named Scopes
- The easiest way to reuse similar scopes is by naming scopes, as those are entered as soon as they are called by their respective name from within a function of time.
- Entering any kind of scope starts a new timer, with the value of 0.
- Exiting any kind of scope happens when there is no timer left.
- A function name can be any literal or string, e.g. `name{}`, `'scope name'{}` (note the string + space), and even `😁😀{}` are valid scope names. It can also start with numbers and whatnot.
- It doesn't matter hot much whitespace is between the name and the `{}`, the name is always
  attached to the next scope.
- You can have nested scopes. For each nesting, only unique scope names are allowed. This means that you can have equal scope names, if they reside on different nestings.

#### Calling Scopes
- There are several ways to call a scope: the literal scope name, their string representation or by using the `?` operation.
- Use whitespace to seperate between different scope callings, because each of those mentioned possibilities always concatenates strings, so `(scope' 'name)` is actually `('scope name')`.
- If there is a scope called `name{}` (or `'name'{}`) you can call it by either `(name)` or `('name')`.
- If using the `?` operation, we try to first and foremost call a function by creating a string. We do that by popping values off the stack until a valid string is made. There may be an invalid string, if the top of stack is `'` but there is no second one, so beware, because in this case the stack will be reverted back to how it was. After either all `?` are used up and replaced by stack values, or a next `?` is a valid operation, we have our callstring.
- If we call a scope but can't find a matching scope, we simply continue, without entering or exiting any scope. Any stack modification is kept.
- When calling equal named scopes from different nesting levels, the nearest one is used, e.g. the one deepest within. The opposite isn't possible; A scope from the outside can't call scopes that reside within another scope.


### A random note
```
all: _~^$\:;.,"#`?&@+-*/=%><!|'
res: _~                       '  // reserved due to reasons
seq:           #    +-*      |
new:   ^$\:;.," `?&@   /=%><!
```

