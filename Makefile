CXX      =  g++
CXXFLAGS = 	-g -gdwarf-2 -std=gnu++17 -Wall -Iinclude -fPIC
LDFLAGS  =	-Llib
AR       =	ar
ARFLAGS	 =	rcs

LIB_HEADERS=	$(wildcard include/sfs/*.h)
LIB_SOURCE=	$(wildcard src/library/*.cpp)
LIB_OBJECTS=	$(LIB_SOURCE:.cpp=.o)
LIB_STATIC=	lib/libafs.a

SHELL_SOURCE=	$(wildcard src/shell/*.cpp)
SHELL_OBJECTS=	$(SHELL_SOURCE:.cpp=.o)
SHELL_PROGRAM=	bin/afssh

all:    $(LIB_STATIC) $(SHELL_PROGRAM)

%.o:	%.cpp $(LIB_HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB_STATIC):		$(LIB_OBJECTS) $(LIB_HEADERS)
	$(AR) $(ARFLAGS) $@ $(LIB_OBJECTS)

$(SHELL_PROGRAM):	$(SHELL_OBJECTS) $(LIB_STATIC)
	$(CXX) $(LDFLAGS) -o $@ $(SHELL_OBJECTS) -lafs

clean:
	rm -f $(LIB_OBJECTS) $(LIB_STATIC) $(SHELL_OBJECTS) $(SHELL_PROGRAM)
