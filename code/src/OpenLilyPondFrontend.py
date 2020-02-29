import urllib.parse
import webbrowser
import sys
fileLocation = sys.argv[1]
f = open(fileLocation, "r")

#print(f.read())
query = f.readline()
print(query)
query = urllib.parse.quote(query)
print(query)
webbrowser.open('https://www.hacklily.org/#src=' + query)