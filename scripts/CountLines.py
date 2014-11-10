import os
import sys

numlines = 0
numFiles = 0

exclude = ['.o', '.bin', '.iso', '.img', '.log', '#', '.dre']

def countLines(path):
	global numlines
	global numFiles
	global exclude

	for filename in os.listdir(path):
		if filename[0] == '.':
			continue

		flag = 0
		for suffix in exclude:
			if filename.endswith(suffix):
				flag = 1
				break
		if flag == 1:
			continue

		print("Checking " + filename)
		if os.path.isdir(filename):
			countLines(path + '/' + filename)
		else:
			numlines += sum(1 for line in open(path + '/' + filename))
			numFiles += 1

def main():
	path = '.'

	if len(sys.argv) > 1:
		path = sys.argv[1]

	print("Counting lines in path: " + path + "\n")
	countLines(path)

	print("[+] " + str(numlines) + " LOC " + str(numFiles) + " files.")

if __name__ == '__main__':
	main()