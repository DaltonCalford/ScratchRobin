/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include "core/project.h"

namespace scratchrobin {

TEST(ProjectGovernanceTest, EnforcesEnvironmentRolesAndApprovals) {
    Project project;
    project.config.governance.review_policy.min_reviewers = 2;
    ProjectConfig::GovernanceEnvironment env;
    env.id = "prod";
    env.allowed_roles = {"admin"};
    env.approval_required = true;
    env.min_reviewers = 1;
    project.config.governance.environments.push_back(env);

    Project::GovernanceContext context;
    context.environment_id = "prod";
    context.role = "designer";
    context.approvals = 2;
    auto decision = project.EvaluateGovernance(context);
    EXPECT_FALSE(decision.allowed);

    context.role = "admin";
    context.approvals = 1;
    decision = project.EvaluateGovernance(context);
    EXPECT_FALSE(decision.allowed);

    context.approvals = 2;
    decision = project.EvaluateGovernance(context);
    EXPECT_TRUE(decision.allowed);
}

TEST(ProjectGovernanceTest, ApproveObjectWithGovernanceRespectsDecision) {
    Project project;
    project.config.governance.review_policy.min_reviewers = 1;
    ProjectConfig::GovernanceEnvironment env;
    env.id = "prod";
    env.allowed_roles = {"admin"};
    env.approval_required = true;
    env.min_reviewers = 1;
    project.config.governance.environments.push_back(env);

    auto obj = project.CreateObject("table", "customers", "public");
    ASSERT_TRUE(obj != nullptr);

    Project::GovernanceContext context;
    context.environment_id = "prod";
    context.role = "designer";
    context.approvals = 1;
    std::string reason;
    EXPECT_FALSE(project.ApproveObjectWithGovernance(obj->id, "user", context, &reason));

    context.role = "admin";
    EXPECT_TRUE(project.ApproveObjectWithGovernance(obj->id, "admin", context, &reason));
    EXPECT_EQ(obj->GetState(), ObjectState::APPROVED);
}

} // namespace scratchrobin
