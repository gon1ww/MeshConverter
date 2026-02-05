#pragma once

#include <exception>
#include <string>
#include "MeshTypes.h"

/**
 * @brief Mesh exception class
 */
class MeshException : public std::exception {
private:
    MeshErrorCode errorCode_;
    std::string errorMsg_;

public:
    /**
     * @brief Constructor
     * @param code Error code
     * @param msg Error message
     */
    MeshException(MeshErrorCode code, const std::string& msg);

    /**
     * @brief Get error code
     * @return Error code
     */
    MeshErrorCode getErrorCode() const;

    /**
     * @brief Get error message
     * @return Error message
     */
    const char* what() const noexcept override;
};
