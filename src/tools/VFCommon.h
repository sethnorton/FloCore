
/*************************************************************************
*
* VULCANFORMS CONFIDENTIAL
* __________________
*  Copyright, VulcanForms Inc.
*  [2016] - [2021] VulcanForms Incorporated
*  All Rights Reserved.
*
*  "VulcanForms", "Vulcan", "Fusing the Future"
*       are trademarks of VulcanForms, Inc.
*
* NOTICE:  All information contained herein is, and remains
* the property of VulcanForms Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to VulcandForms Incorporated
* and its suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from VulcanForms Incorporated.
*/
#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using std::min;
using std::max;
using std::move;
using std::swap;
using std::make_unique;
using std::make_shared;

namespace fs = std::filesystem;

inline bool floatsEqual(const float a, const float b) {
    // Comparison based on: https://stackoverflow.com/questions/4548004/how-to-correctly-and-standardly-compare-floats
    return (fabs(a - b) <= FLT_EPSILON * max(fabs(a), fabs(b)));
}

