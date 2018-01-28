top="$(CURDIR)"/

test_SOURCES=computer.cpp register.cpp input_output_unit.cpp test_computer.cpp test_opcodes.cpp test_input_output.cpp test.cpp
test_OBJ= computer.o register.o input_output_unit.o test_computer.o test_opcodes.cpp test_input_output.o test.o
CXX= g++ -g -Wall
CXX_COMPILE=$(CXX) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
CXX_DEPENDENCIES=-Wp,-MD,.deps/$(*F).P
CXX_LINK=$(CXX) $(CFLAGS) $(LDFLAGS) -L.
LDDEPS= -lboost_unit_test_framework -DBOOST_TEST_DYN_LINK
VERSION=1.0
DISTDIR=$(top)IBM650-$(VERSION)
top_builddir = 

DEP_FILES=.deps/computer.P .deps/register.P .deps/drum.P .deps/test_computer.P .deps/test_opcodes.P .deps/test.P .deps/register.P .deps/computer.P .deps/drum.P .deps/input_output_unit.P .deps/test_input_output.P

all: test

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)
-include $(DEP_FILES)

%.o: %.cpp
	@echo '$(CXX_COMPILE) -c $<'; \
	$(CXX_COMPILE) $(CXX_DEPENDENCIES) -o $@ -c $<

test: $(test_OBJ)
	$(CXX_LINK) -o $@ $^ $(LDDEPS)

tags: 


clean:
	rm -f *.mod *.o *.obj .deps/*.P .lo

.PHONY: dist

dist:
	rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	tar -cvzf $(DISTDIR).tar.gz $(DISTDIR)
	rm -rf $(DISTDIR)
