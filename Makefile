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

SOURCES=main.cpp Component.cpp Configuration.cpp generated.cpp Input.cpp Output.cpp CmakeRegen.cpp Analysis.cpp

all: cpp-dependencies

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(COMPILER) -c -Wall -Wextra -Wpedantic -o $@ $< -std=$(STANDARD) -O3

cpp-dependencies: $(patsubst %.cpp,obj/%.o,$(SOURCES))
	$(COMPILER) -o $@ $^ $(LDFLAGS) -O3 

clean:
	rm -rf obj cpp-dependencies

install:
	cp cpp-dependencies /usr/bin
