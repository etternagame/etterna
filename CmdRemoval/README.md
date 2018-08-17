# CmdRemovalTool

In order for all of your stuff to work on Etterna 0.56 and onwards, you will need to replace every cmd call in your lua files(most likely your noteskins files), and this tool makes this process easier.

## Requirements

Python 3.5 or superior, available at: https://www.python.org/downloads/release/python-363/
The actual script, available at: https://raw.githubusercontent.com/caiohsr14/etterna/master/CmdRemoval/sure.py (right click - save as sure.py)

## Usage Guide

First you move the downloaded script to the root folder of your game, or whatever folder you want to replace the cmds, in this example I'm using my noteskins folder:
![image](img/image1.png?raw=true)

Then open your preferred command line tool and navigate to your folder, if you don't have any you can either open Windows Command Prompt, Windows Powershell or Git Bash
For this one I'm going to use the Windows Command Prompt:
![image](img/image2.png?raw=true)

From here, we simply call the script:
```
python sure.py
```

And then it will list all the files that were(or should be) changed:
![image](img/image3.png?raw=true)

And it's done, congrats.

## Troubleshooting

If you have any questions or if anything broke you can find me (Frustration#2297) on the Etterna Dev Group discord
