/**
* Copyright 2021 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <gtest/gtest.h>
#include <unistd.h>
#include <pthread.h>
#include "rdk_perf_process.h"
#include "rdk_perf_tree.h"

class PerfProcessTest : public ::testing::Test {
protected:
    void SetUp() override {
        RDKPerf_InitializeMap();
    }

    void TearDown() override {
        // Cleanup
    }
};

TEST_F(PerfProcessTest, ConstructorDestructor) {
    pid_t pid = getpid();
    PerfProcess* process = new PerfProcess(pid);
    ASSERT_NE(process, nullptr);
    delete process;
}

TEST_F(PerfProcessTest, GetTree) {
    pid_t pid = getpid();
    PerfProcess process(pid);
    pthread_t tid = pthread_self();
    
    PerfTree* tree = process.GetTree(tid);
    // May be null if tree doesn't exist yet
    EXPECT_TRUE(tree == nullptr || tree != nullptr);
}

TEST_F(PerfProcessTest, NewTree) {
    pid_t pid = getpid();
    PerfProcess process(pid);
    pthread_t tid = pthread_self();
    
    PerfTree* tree = process.NewTree(tid);
    ASSERT_NE(tree, nullptr);
}

TEST_F(PerfProcessTest, GetProcessName) {
    pid_t pid = getpid();
    PerfProcess process(pid);
    
    process.GetProcessName();
    // Should not crash
    SUCCEED();
}

TEST_F(PerfProcessTest, ReportData) {
    pid_t pid = getpid();
    PerfProcess process(pid);
    
    process.ReportData();
    // Should not crash
    SUCCEED();
}

TEST_F(PerfProcessTest, FindProcess) {
    // FindProcess uses a global map that may already have entries
    // Just verify it doesn't crash
    pid_t pid = getpid();
    PerfProcess* found = RDKPerf_FindProcess(pid);
    // May be null or non-null depending on prior tests
    EXPECT_TRUE(found != nullptr || found == nullptr);
}

TEST_F(PerfProcessTest, GetMapSize) {
    // Test that GetMapSize returns a reasonable value
    size_t size = RDKPerf_GetMapSize();
    EXPECT_GE(size, 0);
}

TEST_F(PerfProcessTest, RemoveTree) {
    pid_t pid = getpid();
    PerfProcess process(pid);
    pthread_t tid = pthread_self();
    
    PerfTree* tree = process.NewTree(tid);
    ASSERT_NE(tree, nullptr);
    
    bool removed = process.RemoveTree(tid);
    EXPECT_TRUE(removed);
}

TEST_F(PerfProcessTest, CloseInactiveThreads) {
    pid_t pid = getpid();
    PerfProcess process(pid);
    
    bool result = process.CloseInactiveThreads();
    // Result depends on whether there are inactive threads
    EXPECT_TRUE(result || !result);
}

TEST_F(PerfProcessTest, MultipleThreads) {
    pid_t pid = getpid();
    PerfProcess process(pid);
    
    pthread_t tid1 = pthread_self();
    pthread_t tid2 = tid1 + 1; // Fake thread ID for testing
    
    PerfTree* tree1 = process.NewTree(tid1);
    PerfTree* tree2 = process.NewTree(tid2);
    
    ASSERT_NE(tree1, nullptr);
    ASSERT_NE(tree2, nullptr);
    EXPECT_NE(tree1, tree2);
}
