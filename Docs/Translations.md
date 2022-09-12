# Translations

Translation support has always been a part of Stepmania to some extent. With Etterna 0.67.0+ we have opened up the doors to translating most of Til Death, and Etterna 0.71.3+ adds support for translation of Rebirth, as well as some more of the base game's strings.

* [Supported Languages](#Languages)
* [The Process](#Process)
* [Theme Support](#Themes)
* [Finishing](#Finishing)

## Languages

Language support is hit and miss. Technically, Etterna supports this entire list of languages: [Here](http://www.loc.gov/standards/iso639-2/php/code_list.php)

Only the 2 letter codes are implemented. The list can also be viewed directly [Here](https://github.com/etternagame/etterna/blob/5b4f5a5138f9e0d351c4ba2961fde5c69697f936/src/RageUtil/Utils/RageUtil.cpp#L504-L576)

Find your language in the list if it exists and keep track of its 2 letter code.

Next is knowing whether or not your language's script or font is supported. In most cases, it is. If your language can entirely be represented with the English alphabet, there is almost no reason there would be any error. Some scripts are unsupported because a font for them does not exist. Some letters in some languages or some combinations of letters are unsupported either due to them missing from the font or some other bug.

Figuring out if your language is supported at the moment is mostly trial and error. If there are any errors with particular scripts, letters, or combinations of letters, let us know and we can look at ways to fix it possibly.

## Process

* Determine your 2 letter language combo from the lists found above.
* Locate `/Themes/_fallback/Languages/en.ini` and `/Themes/Til Death/Languages/en.ini` or `/Themes/Rebirth/Languages/en.ini`
* Make a copy of both files and leave them in the same respective `Languages` folders. Rename the copies to use your 2 letter language combination. For example, Norwegian would be `no.ini`
* Begin translating line by line in the new files.

* Testing can begin immediately as you translate. Keep in mind the following helpful things:
  * Change language from the Display/Appearance Options Screen
    * In case you get lost: Main Menu -> Down 2 (Options) -> Down 5 (Display Options) -> Down 1 (Appearance Options) -> You are now hovering Languages
  * `F2` to reload cached files and textures
  * `Shift + F2` to reload all `.ini` files including metrics and Languages
  * `Ctrl + F2` to reload all Theme Scripts files
  * After reloading the Languages with the above combo, the current Screen must be restarted to view changes either by re-entering it or by pressing `F3 + F6 + 2`
  * A small number of strings are in Overlay Screens which must be reloaded with `F3 + F6 + 7`
  * If a string does not update when you reload with `Shift + F2`, try `Ctrl + F2`
  * A full game restart after changing language will refresh everything if any of the above happens to fail.
  
Many lines in both language files go unused and may even make absolutely no sense. In the future, we may reduce this extra garbage.

If you run into any lines to translate that you would like more context to in order to provide a correct translation, let us know.

## Themes

Spawncamping-wallhack does not feature this full translation support, but could in the future. 

For your own theme, these translations can be done the exact same way. The Language files accessible from your theme are only those in `_fallback/Languages` and `yourtheme/Languages`. This means that the Til Death or Rebirth strings are not available, but that does not stop you from taking them at your own leisure.

Adding additional strings to Til Death or any theme involves creating a new line in a designated section and then calling it with `THEME:GetString("section", "string name")`. We try to reduce calls to this by storing them in a table instead of loading them multiple times per Actor. This `GetString` function will look for the current language in the theme and fallback. Then it will look in the `en.ini` in the same order. If it finds nothing in all cases, then you get a missing string error.

## Finishing

Once your translation is finished, you can create a pull request into our develop branch and have the work reviewed. Or you can share it privately. We would love to have more translations instantly available in the game.

To create a pull request, you must first fork the repository to your own Github account. Then you can directly upload the files to your fork in the right place. After that, you make a comparison with our repository and create the pull request there.
