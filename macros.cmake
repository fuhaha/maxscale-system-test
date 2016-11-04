# Build the MariaDB Connector-C

include(ExternalProject)

set(MARIADB_CONNECTOR_C_REPO "https://github.com/MariaDB/mariadb-connector-c.git"
  CACHE STRING "MariaDB Connector-C Git repository")

# Use version 2.3
set(MARIADB_CONNECTOR_C_TAG "v2.3.0"
  CACHE STRING "MariaDB Connector-C Git tag")

ExternalProject_Add(connector-c
  GIT_REPOSITORY ${MARIADB_CONNECTOR_C_REPO}
  GIT_TAG ${MARIADB_CONNECTOR_C_TAG}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/connector-c/install
  BINARY_DIR ${CMAKE_BINARY_DIR}/connector-c
  INSTALL_DIR ${CMAKE_BINARY_DIR}/connector-c/install
  UPDATE_COMMAND "")

set(MARIADB_CONNECTOR_INCLUDE_DIR
  ${CMAKE_BINARY_DIR}/connector-c/install/include/mariadb CACHE INTERNAL "")
set(MARIADB_CONNECTOR_STATIC_LIBRARIES
  ${CMAKE_BINARY_DIR}/connector-c/install/lib/mariadb/libmariadbclient.a
  CACHE INTERNAL "")
set(MARIADB_CONNECTOR_LIBRARIES
  ${CMAKE_BINARY_DIR}/connector-c/install/lib/mariadb/libmariadbclient.a
  CACHE INTERNAL "")

include_directories(${MARIADB_CONNECTOR_INCLUDE_DIR})
