#pragma once

#include <exception>
#include <string>
#include "MeshTypes.h"

/**
 * @brief 网格异常类
 */
class MeshException : public std::exception {
private:
    MeshErrorCode errorCode_;
    std::string errorMsg_;

public:
    /**
     * @brief 构造函数
     * @param code 错误码
     * @param msg 错误信息
     */
    MeshException(MeshErrorCode code, const std::string& msg);

    /**
     * @brief 获取错误码
     * @return 错误码
     */
    MeshErrorCode getErrorCode() const;

    /**
     * @brief 获取错误信息
     * @return 错误信息
     */
    const char* what() const noexcept override;
};
