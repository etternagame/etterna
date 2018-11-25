# QuickStart

A lua file is plain text. To create and edit these you need a plain text editor such as notepad.

You can use any text editor, but to make your life easier, we recommend that you use [vscode](https://code.visualstudio.com/Download). Extensions you may want to use with [vscode](https://code.visualstudio.com/Download) would definitely be the [vscode-lua](https://marketplace.visualstudio.com/items?itemName=trixnz.vscode-lua) generic lua extension and the [ettlua](https://marketplace.visualstudio.com/items?itemName=Nick12.ettlua-for-vscode) extension.

If you have no programming language experience whatsoever or you are not familiar with lua and don't think you would be comfortable infering the syntax, it is recommended that you read some lua book or material (Like [Programming In Lua](https://www.lua.org/pil/contents.html), chapter 1 being optional, chapters 2 to 5 recommended and 13,19 and 20 also optional. You're free to read the whole book, but you will probably not get a lot of useful things for lua in this game from the rest).

## Basic Structure

The game defines a number of `Screens`. It's possible to define custom `Screens`, but that's not inmediately relevant. Each `Screen` is a container for `Actors`. Pretty much every Drawable thing is an `Actor`. A container for `Actors` is actually called an `ActorFrame` (So, `Screens` are `ActorFrames`). Lua comes into play in the game when loading `Screens`. Some lua code is executed when initializing `Screens`, and it tells the game which `Actors` it needs to create and add to it. It can also registers a number of `Commands`, which are just lua functions executed on certain events (Usually called by the game engine). Lua code is also executed when the game is executed, on startup, usually to initialize whatever global state it needs to use later, and to define functions globally so they can be available everywhere (Note: Usually it's recommended to "namespace" your global functions, putting them under a table, so as to not pollute the global namespace). A lua file executed at startup is called a `Script` within the context of this game. Sometimes, Screen lua files are called `BGAnimations`, or "BackGround Animations" (Sometimes abbreviated BGAnims).

## Screens

I mentioned lua code can be executed on `Screen` initialization. However, `Screens` have a second component to them that is of relatively big importance to someone dealing with BGAnims: metrics.ini
