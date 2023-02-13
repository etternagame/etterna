# Contributing

Anyone is welcome to contribute! These are the areas where help is needed the most.

* [C++](#c)
* [Lua](#lua)
* [Documentation](#documentation)
* [Translating](#translating)
* [Packaging](#packaging)
* [Graphic Design](#graphic-design)
* [Chart Making](#chart-making)

## C++

If you have *any* experience with C++ (Or even C and wanna learn a bit of C++) your help would be much appreciated!
There's **A LOT** of things that need doing. If you don't have any specific thing you want to work on, here are some ideas:

* [GitHub Issues](https://github.com/etternagame/etterna/issues)
* [Coverity Defects](https://scan.coverity.com/projects/etternagame-etterna) (Currently these are private, you can request access with an email, we might make this public in the future)
* Cleaning up the codebase in general (There's a lot of things that would be nice to see done, like replacing usage of RString with std::string)
* If you know how to use them and see a good place for them, some usage of modern C++ features like smart pointers might be good to see in the project

## Lua

Lua is a fairly simple language. If you have experience in any other programming language, you'll probably be able to get the basics of its syntax in under an hour. The [Lua Syntax Guide](https://www.lua.org/pil/contents.html) is a good place to get started.

Lua is used in Etterna for Themes and Noteskins.

Noteskins control the way the notes are drawn (Most noteskins have very similar logic and only different images, but noteskins have the power to do a lot more, and some take advantage of that).

You can think of themes as the front end of the game. The way each screen is drawn, how screen branching (Changing) happens, and other things are controlled by the theme. A theme can even create new screens from scratch, catch inputs (Both mouse and keyboard), and other things. They're fairly powerful.

Currently, there's a lack of alternative themes (To the default one, Til Death) for Etterna. [Spawncamping-Wallhack](https://github.com/ca25nada/spawncamping-wallhack) is the only complete theme that supports Etterna as far as I know.

## Documentation

The project is sorely lacking in documentation.

The current aim is to document the Lua stuff in https://etternagame.github.io/Lua-For-Etterna/API/Lua.xml (This is forked from https://quietly-turning.github.io/Lua-For-SM5/Luadoc/Lua.xml, a community-made reference for SM5 Lua).

We intend to get the C++ documentation done in https://etternagame.github.io/wiki/

There is a general wiki with information that can be useful to end users (https://wiki.etternaonline.com/).

## Translating

A guide on this can be found in [the translations document](Translations.md).

## Packaging

A few people have complained about the lack of packages for Linux in general. If you'd like to, you can try working on a package for your favorite distro.

## Graphic Design

A lot of things in the game have been done in a hurry to simply provide a basic UI so things can be used. We usually mostly care about features working. Mockups for new designs, improved gfx (Images) and other ideas are incredibly welcome. This goes for both the default theme and the website, etternaonline. If you're interested, you could ask around in the etternaonline discord. Also, concepts for completely new themes would be interesting to see (In case someone who knows how to implement one decides to work on it).

## Chart Making

As the content in the game is completely community-driven, new charts and packs keep the game fresh. They make old time players come back for a day. Even if it doesnt seem like much, every chart makes the game as a whole more valuable. You can get started by clicking "Editor" within the client.
