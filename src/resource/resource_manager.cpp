#include "resource_manager.hpp"


/**
* @brief Get a handle to an already-loaded resource.
* @tparam T The type of resource.
* @param id The resource ID.
* @return A handle sharing ownership of the resource, or an invalid handle if not found.
*/

/**
* @brief Get the current reference count of a resource.
* @tparam T The type of resource.
* @param id The resource ID.
* @return The number of outstanding references, or 0 if the resource doesn't exist.
*/

/**
* @brief Check if a resource exists.
* @tparam T The type of resource.
* @param id The resource ID.
* @return True if the resource exists, false otherwise.
*/

/**
* @brief Release the manager's own reference to a resource.
* @tparam T The type of resource.
* @param id The resource ID.
* @return True if the resource existed, false otherwise.
*/
