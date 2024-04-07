/******************************************************************************
 * Copyright 2023 Shenzhen Hangsheng Electronics Co., Ltd. All Rights Reserved.
 *
 * This software and documentation contain confidential and proprietary
 * information which is the property of HSAE Co., Ltd. Unauthorized use,
 * copying, or distribution of this software, its source code, or its
 * documentation, via any medium, is strictly prohibited without the prior
 * written consent of HSAE Co., Ltd.
 *
 ******************************************************************************/
#include <utils/Errors.h>
#include <selinux/selinux.h>
#include <android-base/logging.h>

using namespace android;

status_t ForkExecvp(const std::vector<std::string>& args, int& forkPid, std::vector<std::string>* output = nullptr,
                    security_context_t context = nullptr);
bool IsRecoveryMode();
void initLogger(android::base::LogId /* id */, android::base::LogSeverity severity,
                         const char* tag, const char* /* file */, unsigned int /* line */,
                         const char* message);
void initHandler();
