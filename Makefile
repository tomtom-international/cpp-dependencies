# Copyright (C) 2012-2016. TomTom International BV (http://tomtom.com).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

COMPILER?=g++
LDFLAGS=-lboost_filesystem -lboost_system

STANDARD=c++11

SOURCES=Component.cpp Configuration.cpp generated.cpp Input.cpp Output.cpp CmakeRegen.cpp Analysis.cpp

TESTS=AnalysisCircularDependencies.cpp

all: cpp-dependencies

debug: cpp-dependencies-g

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(COMPILER) -c -Wall -Wextra -Wpedantic -o $@ $< -std=$(STANDARD) -O3 -MMD

obj-g/%.o: src/%.cpp
	@mkdir -p obj-g
	$(COMPILER) -c -Wall -Wextra -Wpedantic -o $@ $< -std=$(STANDARD) -g

obj/%.coverage.o: src/%.cpp
	@mkdir -p obj
	$(COMPILER) -c -Wall -Wextra -Wpedantic -o $@ $< -std=$(STANDARD) -O3 -MMD --coverage

test/obj/%.coverage.o: test/%.cpp
	@mkdir -p test/obj
	$(COMPILER) -c -Wall -Wextra -Wpedantic -o $@ $< -std=$(STANDARD) -Isrc -O3 -MMD --coverage

cpp-dependencies: $(patsubst %.cpp,obj/%.o,$(SOURCES)) obj/main.o
	$(COMPILER) -o $@ $^ $(LDFLAGS) -O3 

cpp-dependencies-g: $(patsubst %.cpp,obj-g/%.o,$(SOURCES)) obj-g/main.o
	$(COMPILER) -o $@ $^ $(LDFLAGS) -g


cpp-dependencies-unittests: $(patsubst %.cpp,obj/%.coverage.o,$(SOURCES)) $(patsubst %.cpp,test/obj/%.coverage.o,$(TESTS))
	$(COMPILER) -o $@ $^ $(LDFLAGS) -O3 -lgtest -lgtest_main -pthread --coverage

unittest: cpp-dependencies-unittests
	./cpp-dependencies-unittests >unittest

clean:
	rm -rf obj obj-g cpp_dependencies cpp-dependencies-g

install:
	cp cpp-dependencies /usr/bin

-include $(patsubst %.cpp,obj/%.o.d,$(SOURCES)) $(patsubst %.cpp,test/obj/%.o.d,$(TESTS))

