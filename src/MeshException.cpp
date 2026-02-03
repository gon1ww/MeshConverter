#include "MeshException.h"

/**
 * @brief 构造函数
 * @param code 错误码
 * @param msg 错误信息
 */
MeshException::MeshException(MeshErrorCode code, const std::string& msg)
    : errorCode_(code), errorMsg_(msg) {
}

/**
 * @brief 获取错误码
 * @return 错误码
 */
MeshErrorCode MeshException::getErrorCode() const {
    return errorCode_;
}

/**
 * @brief 获取错误信息
 * @return 错误信息
 */
const char* MeshException::what() const noexcept {
    return errorMsg_.c_str();
}
