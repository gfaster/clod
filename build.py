# Run with: python3 build.py
import os
import platform
import json

# (1)==================== COMMON CONFIGURATION OPTIONS ======================= #
COMPILER="g++ -std=c++17"   # The compiler we want to use 
                                #(You may try g++ if you have trouble)
SOURCE="./src/*.cpp"    # Where the source code lives
EXECUTABLE="project"        # Name of the final executable
# ======================= COMMON CONFIGURATION OPTIONS ======================= #

# (2)=================== Platform specific configuration ===================== #
# For each platform we need to set the following items
ARGUMENTS=""            # Arguments needed for our program (Add others as you see fit)
INCLUDE_DIR=""          # Which directories do we want to include.
LIBRARIES=""            # What libraries do we want to include

if platform.system()=="Linux":
    # ARGUMENTS="-D LINUX -g -fsanitize=address -fsanitize=undefined" # -D is a #define sent to preprocessor
    ARGUMENTS="-D LINUX -O2"
    INCLUDE_DIR="-I ./include/ -I ./../thirdparty/glm/ -I ./../libinstall/include/"
    LIBRARIES="-L ./../libinstall/lib/ -lmetis -lGKlib -lSDL2 -ldl"
elif platform.system()=="Darwin":
    print("no mac, try linux.")
    exit(1)
elif platform.system()=="Windows":
    print("no window, try linux.")
    exit(1)
# (2)=================== Platform specific configuration ===================== #



# (3)=================== Build METIS =================== #

rebuild_metis = False

if not os.path.isdir("../metis"):
    rebuild_metis = True
    print("============v (Cloning METIS) v===========================")
    os.system("git clone https://github.com/KarypisLab/METIS.git ../metis")

if not os.path.isdir("../gklib"):
    rebuild_metis = True
    print("============v (Cloning GKlib) v===========================")
    os.system("git clone https://github.com/KarypisLab/GKlib.git ../gklib")

# gdbflag = "gdb=1 assert=1 assert2=1"
gdbflag = ""
cwd = os.getcwd()
installdir = cwd + "/../libinstall"
rebuild_gklib_cmd = f"""cd {cwd}/../gklib ;
make distclean ;
make config prefix={installdir} {gdbflag} ;
make install ;
cd {cwd}"""

rebuild_metis_cmd = f"""cd {cwd}/../metis ;
make distclean ;
make config prefix={installdir} {gdbflag} i64=1 r64=1;
make install ;
cd {cwd}"""

if rebuild_metis:
    print("============v ((re)building METIS) v===========================")
    os.system(rebuild_gklib_cmd)
    os.system(rebuild_metis_cmd)



# (3)=================== Build METIS =================== #



# (4)====================== Building the Executable ========================== #
# Build a string of our compile commands that we run in the terminal
compileString=COMPILER+" "+ARGUMENTS+" -o "+EXECUTABLE+" "+" "+INCLUDE_DIR+" "+SOURCE+" "+LIBRARIES
# Print out the compile string
# This is the command you can type
print("============v (Command running on terminal) v===========================")
print("Compilng on: "+platform.system())
print(compileString)
print("========================================================================")
# Run our command 
# Here I am using an exit_code so you can
# also compile & run in one step as
# python3 build.py && ./lab
# If compilation failes, ./lab will not run.
exit_code = os.system(compileString)


with open('compile_commands.json', 'w') as f:
    f.write(json.dumps([{'directory': os.getcwd(), 'command':compileString, 'file': './src/main.cpp'}]))

exit(0 if exit_code==0 else 1)
# ========================= Building the Executable ========================== #


# Why am I not using Make?
# 1.)   I want total control over the system. 
#       Occasionally I want to have some logic
#       in my compilation process (like searching for missing files).
# 2.)   Realistically our projects are 'small' enough 
#       this will not matter.
# 3.)   Feel free to implement your own make files or autogenerate it from this
#       script
# 4.)   It is handy to know Python

