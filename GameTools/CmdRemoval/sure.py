import io
import os

for root, dirs, files in os.walk(os.getcwd()):
	for file in files:
		if (".lua" in file):
			enc = "utf8"
			fullpath = os.path.join(root, file)
			try:
				hasCmd = "cmd" in io.open(fullpath, encoding=enc).read()
			except UnicodeDecodeError:
				enc = "windows-1252"
				hasCmd = "cmd" in io.open(fullpath, encoding=enc).read()
			if (hasCmd):
				print("cmd found:", fullpath)
				with io.open(fullpath, "r", encoding=enc) as f:
					lines = f.readlines()
				for i, line in enumerate(lines):
					if not line.lstrip().startswith("-"):
						if "cmd" in line.split("--")[0]:
							indent = line[:len(line) - len(line.lstrip())]
							cmt = " " + line[line.rfind("--"):].strip() if len(line.split("--")) > 1 else ""
							line = line.split("--")[0]
							oneLiner = "{" in line and "}" in line
							ending = line.strip()[line.strip().index("}"):] if oneLiner else line.strip()[-1]
							if ending == ')':
								ending = ""
							init = line.split("cmd")[0]
							args = line.split("cmd")[1]
							idx = args[:args.rfind(')')].rfind(')') if "RunCommandsOnChildren" in init else args[:args[:args.rfind(')')].rfind(')')].rfind(')') if init[-1] == '(' else args.rfind(')')
							args = args[1:idx].split(";") if idx - 1 > 0 else ""
							newArg = ""
							if len(args) > 0:
								for arg in args:
									if (arg != ""):
										if (len(arg.split(",", 1)) == 1):
											newArg += ":" + arg + "()"
										else:
											newArg += ":" + arg.split(",", 1)[0].strip() + "(" + arg.split(",", 1)[1].strip() + ")"
								if oneLiner:
									init = ("{\n" + indent + "	").join(init.split("{"))
									lines[i] = indent + cmt + "\n" + init + "function(self)\n" + indent + "		self" + newArg + "\n" + indent + "	end\n" + indent + ending + "\n"
								elif "RunCommandsOnChildren" in init:
									lines[i] = init + "function(self) self" + newArg + " end" + ")" + ending + cmt + "\n"
								elif init[-1] == '(':
									lines[i] = indent + line[line.rfind('(')+1:line.rfind(')')] + newArg + ending + cmt + "\n"
								else:
									lines[i] = init + "function(self)\n" + indent + "	self" + newArg + cmt + "\n" + indent + "end" + ending + "\n"
							else:
								lines[i] = ""
				with io.open(fullpath, "w", encoding=enc) as f:
					lines = "".join(lines)
					f.write(lines)
