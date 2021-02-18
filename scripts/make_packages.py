import os

files = os.listdir("../packages")

for name in files:
    os.system("../packages/" + name + "/install.sh")