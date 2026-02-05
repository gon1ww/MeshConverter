#include "MeshException.h"

/**
 * @brief Constructor
 * @param code Error code
 * @param msg Error message
 */
MeshException::MeshException(MeshErrorCode code, const std::string& msg)
    : errorCode_(code), errorMsg_(msg) {
}

/**
 * @brief Get error code
 * @return Error code
 */
MeshErrorCode MeshException::getErrorCode() const {
    return errorCode_;
}

/**
 * @brief Get error message
 * @return Error message
 */
const char* MeshException::what() const noexcept {
    return errorMsg_.c_str();
}
