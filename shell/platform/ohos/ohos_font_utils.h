// Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OHOS_FONT_UTILS_H
#define OHOS_FONT_UTILS_H

#include <dirent.h>
#include <string>
namespace flutter {

inline bool endsWith(std::string str, std::string suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

inline std::string getFontFileName(std::string path)
{
    std::string fontFamilyName = "";
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(path.c_str())) == nullptr) {
        return fontFamilyName;
    }
    while ((ent = readdir(dir)) != nullptr) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        if (endsWith(ent->d_name, ".ttf")) {
            fontFamilyName = ent->d_name;
            break;
        }
    }
    closedir(dir);
    return fontFamilyName;
}

inline bool isFontDirValid(std::string path)
{
    DIR* dir;
    struct dirent* ent;
    bool isFlagFileExist = false;
    bool isFontDirExist = false;
    if ((dir = opendir(path.c_str())) == nullptr) {
        if (errno == ENOENT) {
            FML_DLOG(ERROR) << "ERROR ENOENT";
        } else if (errno == EACCES) {
            FML_DLOG(ERROR) << "ERROR EACCES";
        } else {
            FML_DLOG(ERROR) << "ERROR Other";
        }
        return false;
    }
    while ((ent = readdir(dir)) != nullptr) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        if (strcmp(ent->d_name, "flag") == 0) {
            isFlagFileExist = true;
        } else if (strcmp(ent->d_name, "fonts") == 0) {
            isFontDirExist = true;
        }
    }
    closedir(dir);
    if (isFlagFileExist && isFontDirExist) {
        FML_DLOG(INFO) << "font path exist" << path;
        return true;
    }
    return false;
}

inline std::string OHOSCheckFontSource()
{
    std::string path = "/data/themes/a/app";
    if (!isFontDirValid(path)) {
        path = "/data/themes/b/app";
        if (!isFontDirValid(path)) {
            return "";
        }
    }
    path = path.append("/fonts/");
    std::string fileName = getFontFileName(path);
    if (fileName.empty()) {
        return "";
    }
    path = path.append(fileName);
    return path;
}
}  // namespace flutter

#endif /* OHOS_FONT_UTILS_H */
