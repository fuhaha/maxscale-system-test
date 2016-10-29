# Some constants that make changing the connector easier
set(JDBC_JAR ${CMAKE_CURRENT_SOURCE_DIR}/mariadb-java-client-1.5.4.jar CACHE INTERNAL "")
set(MXS_JAR ${CMAKE_CURRENT_BINARY_DIR}/maxscale_java.jar CACHE INTERNAL "")
set(TEST_JARPATH "${MXS_JAR}:${JDBC_JAR}" CACHE INTERNAL "")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/MaxScaleConfiguration.java.in ${CMAKE_CURRENT_BINARY_DIR}/MaxScaleConfiguration.java @ONLY)
add_jar(maxscale_java SOURCES MaxScaleConnection.java MaxScaleConfiguration.java
  INCLUDE_JARS mariadb-java-client-1.5.4.jar)
add_subdirectory(test1)
add_subdirectory(prep_stmt)
