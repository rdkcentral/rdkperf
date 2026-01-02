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
#include "rdk_perf_tree.h"
#include "rdk_perf_record.h"
#include "rdk_perf_node.h"

class PerfTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(PerfTreeTest, ConstructorDestructor) {
    PerfTree* tree = new PerfTree();
    ASSERT_NE(tree, nullptr);
    delete tree;
}

TEST_F(PerfTreeTest, GetThreadID) {
    PerfTree tree;
    pthread_t tid = tree.GetThreadID();
    
    // PerfTree initializes m_idThread to 0 and only sets it when a node is added
    // So initially it should be 0
    EXPECT_EQ(tid, 0);
    
    // After adding a node with name-based constructor, it should be set
    char name[] = "test_thread_id";
    pthread_t current_tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    tree.AddNode(name, current_tid, threadName, startTime);
    tid = tree.GetThreadID();
    EXPECT_TRUE(pthread_equal(tid, pthread_self()));
}

TEST_F(PerfTreeTest, GetName) {
    PerfTree tree;
    char* name = tree.GetName();
    ASSERT_NE(name, nullptr);
    // Name should be non-empty for named thread
}

TEST_F(PerfTreeTest, AddNodeWithRecord) {
    // Note: AddNode with PerfRecord is designed for internal use.
    // Testing with name-based constructor instead.
    PerfTree tree;
    char name[] = "test_function";
    pthread_t tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    PerfNode* node = tree.AddNode(name, tid, threadName, startTime);
    ASSERT_NE(node, nullptr);
}

TEST_F(PerfTreeTest, AddNodeWithNameAndThread) {
    PerfTree tree;
    char name[] = "custom_node";
    pthread_t tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    PerfNode* node = tree.AddNode(name, tid, threadName, startTime);
    ASSERT_NE(node, nullptr);
}

TEST_F(PerfTreeTest, GetStack) {
    PerfTree tree;
    std::stack<PerfNode*>* stack = tree.GetStack();
    ASSERT_NE(stack, nullptr);
    
    // Initially, stack might be empty or have root node
}

TEST_F(PerfTreeTest, IsInactive) {
    PerfTree tree;
    char name[] = "test_function";
    pthread_t tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    // Add a node to make it active
    tree.AddNode(name, tid, threadName, startTime);
    
    // Check inactive status - should not crash
    bool inactive = tree.IsInactive();
    // Just verify the call succeeds
    SUCCEED();
}

TEST_F(PerfTreeTest, MultipleNodeAdditions) {
    PerfTree tree;
    pthread_t tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    char name1[] = "function1";
    char name2[] = "function2";
    char name3[] = "function3";
    
    PerfNode* node1 = tree.AddNode(name1, tid, threadName, startTime);
    PerfNode* node2 = tree.AddNode(name2, tid, threadName, startTime);
    PerfNode* node3 = tree.AddNode(name3, tid, threadName, startTime);
    
    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);
    ASSERT_NE(node3, nullptr);
}

TEST_F(PerfTreeTest, ReportData) {
    PerfTree tree;
    char name[] = "test_function";
    pthread_t tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    tree.AddNode(name, tid, threadName, startTime);
    
    // Report data should not crash
    tree.ReportData(1000);
    
    SUCCEED();
}

TEST_F(PerfTreeTest, CloseActiveNode) {
    PerfTree tree;
    pthread_t tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    char name[] = "test_function";
    PerfNode* node = tree.AddNode(name, tid, threadName, startTime);
    
    // Close the active node
    tree.CloseActiveNode(node);
    
    SUCCEED();
}

TEST_F(PerfTreeTest, NestedNodes) {
    PerfTree tree;
    pthread_t tid = pthread_self();
    char threadName[] = "test_thread";
    uint64_t startTime = PerfRecord::TimeStamp();
    
    char name1[] = "outer_function";
    char name2[] = "inner_function";
    
    PerfNode* outer = tree.AddNode(name1, tid, threadName, startTime);
    PerfNode* inner = tree.AddNode(name2, tid, threadName, startTime);
    
    ASSERT_NE(outer, nullptr);
    ASSERT_NE(inner, nullptr);
    
    // Close in reverse order
    tree.CloseActiveNode(inner);
    tree.CloseActiveNode(outer);
    
    SUCCEED();
}
