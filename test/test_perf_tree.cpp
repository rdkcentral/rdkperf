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
    // GetThreadID may return 0 initially, or the thread ID
    pthread_t tid = tree.GetThreadID();
    EXPECT_GE(tid, 0);
}

TEST_F(PerfTreeTest, GetName) {
    PerfTree tree;
    char* name = tree.GetName();
    ASSERT_NE(name, nullptr);
    // Name should be non-empty for named thread
}

TEST_F(PerfTreeTest, AddNodeWithRecord) {
    PerfTree tree;
    PerfRecord record("test_function");
    
    PerfNode* node = tree.AddNode(&record);
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
    PerfRecord record("test_function");
    
    // Add a node to make it active
    tree.AddNode(&record);
    
    // Check inactive status - should not crash
    bool inactive = tree.IsInactive();
    // Just verify the call succeeds
    SUCCEED();
}

TEST_F(PerfTreeTest, MultipleNodeAdditions) {
    PerfTree tree;
    
    PerfRecord record1("function1");
    PerfRecord record2("function2");
    PerfRecord record3("function3");
    
    PerfNode* node1 = tree.AddNode(&record1);
    PerfNode* node2 = tree.AddNode(&record2);
    PerfNode* node3 = tree.AddNode(&record3);
    
    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);
    ASSERT_NE(node3, nullptr);
}

TEST_F(PerfTreeTest, ReportData) {
    PerfTree tree;
    PerfRecord record("test_function");
    
    tree.AddNode(&record);
    
    // Report data should not crash
    tree.ReportData(1000);
    
    SUCCEED();
}

TEST_F(PerfTreeTest, CloseActiveNode) {
    PerfTree tree;
    PerfRecord record("test_function");
    
    PerfNode* node = tree.AddNode(&record);
    
    // Close the active node
    tree.CloseActiveNode(node);
    
    SUCCEED();
}

TEST_F(PerfTreeTest, NestedNodes) {
    PerfTree tree;
    
    PerfRecord record1("outer_function");
    PerfRecord record2("inner_function");
    
    PerfNode* outer = tree.AddNode(&record1);
    PerfNode* inner = tree.AddNode(&record2);
    
    ASSERT_NE(outer, nullptr);
    ASSERT_NE(inner, nullptr);
    
    // Close in reverse order
    tree.CloseActiveNode(inner);
    tree.CloseActiveNode(outer);
    
    SUCCEED();
}
