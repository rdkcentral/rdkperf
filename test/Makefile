##
# Copyright 2021 Comcast Cable Communications Management, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#
##
include ../Makefile.Features

CXXFLAGS += -Wno-attributes -Wall -g -fpermissive -std=c++1y -fPIC
CXXFLAGS += $(FEATURE_FLAGS)

CFLAGS = -std=c99 $(CXXFLAGS)

INCLUDES += \
	-I$(PWD)/../src \
	-I$(PWD)/../rdkperf \
	-I/usr/src/googletest/googletest/include

# Libraries to load
LD_FLAGS =  \
    -lpthread -lstdc++

LD_FLAGS += -L$(BUILD_DIR) -lrdkperf -lperftool
LD_FLAGS += -lgtest -lgtest_main

NAME = rdkperf_tests

SRC_DIRS = .

DIR_CREATE = @mkdir -p $(@D)

# Find all the C and C++ files we want to compile
SRCS := $(shell find $(SRC_DIRS) -name test_\*.cpp -or -name test_main.cpp)

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/$(NAME)

$(BUILD_DIR)/%.c.o: %.c
	$(DIR_CREATE)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(DIR_CREATE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LD_FLAGS) 

test: $(BUILD_DIR)/$(NAME)
	@echo "Running tests..."
	$(BUILD_DIR)/$(NAME)

clean:
	rm -f $(OBJS)
	rm -f $(BUILD_DIR)/$(NAME)

